#if defined(ARDUINO_ARCH_STM32)
  #include <systemClockConfig.h>
  HardwareSerial Serial(PA10, PA9);
#else
  #define OLED_ON true
#endif

#include <adf7023.h>
#include <AODV.h>

#ifdef OLED_ON
  #include <Wire.h>
  #include <Adafruit_GFX.h>
  #include <Adafruit_SSD1306.h>
  Adafruit_SSD1306 display(128, 64, &Wire, -1);
#endif  

#ifndef IRAM_ATTR
#define IRAM_ATTR
#endif

// Pin configuration for ADF7023 connection depending on target architecture
#if defined(ARDUINO_ARCH_STM32)
  #define ADF7023_SPI_CS   PA4
  #define ADF7023_SPI_MISO PA6
  #define ADF7023_SPI_MOSI PA7
  #define ADF7023_SPI_SCLK PA5
  
  #define RF_IRQ_PIN        PA8
  #define LED_PIN           PB10
  
  SPIClass adf7023SPI;
  adf7023 ADF7023(ADF7023_SPI_CS, ADF7023_SPI_MISO, SPI_CLOCK_DIV32, &adf7023SPI);
#else
  #define RF_IRQ_PIN        16
  #define LED_PIN           2
  
  adf7023 ADF7023(5, 19, SPI_CLOCK_DIV2);
#endif

uint64_t chipId = 0;      // Unique device identifier (MAC address)
uint64_t targetMac = 0;   // Target MAC address for manual packet transmission

#define CHANNELS          5

#define LED_DELAY         (unsigned long)(500)    // LED indicator duration (ms)
#define FREQ_SWAP_DELAY   (unsigned long)(1500)   // Frequency switching delay (us)

#define RSSI_LIMIT        -90                     // RSSI threshold level for CSMA (dBm)

#define FREQ_BROADCAST_RREQ     863950000         // Frequency used for broadcast RREQs
#define FREQ_RREP               868950000         // Frequency used for RREP responses

// Table of available radio frequencies for frequency hopping
long frequencies_table[25] = { 865950000, 864550000, 865350000, 869550000, 867750000, 
                               864750000, 866150000, 865150000, 866750000, 865550000, 
                               869750000, 866950000, 867350000, 867950000, 866350000, 
                               868350000, 868150000, 869950000, 863350000, 867550000, 
                               864950000, 867150000, 865750000, 863150000, 866550000     
                             };

int channelSwitchCounter = 0;
long rx_frequencies[CHANNELS];

uint8_t rx_packet[300];
uint8_t len;

unsigned long ledTimer = 0;
bool ledST = false;

unsigned long listenTimer = 0;
volatile bool packetWait = false;
volatile bool packetFound = false;

// Interrupt service routine (ISR) for the ADF7023 transceiver
void IRAM_ATTR rf_irq() {
    packetFound = true;
}

AODV aodv = AODV();

/**
 * Calculate active communication channels based on the node's MAC address
 */
long* calcFrequenciesByMAC(uint64_t mac){
  static long channelsMAC[3] = {0, 0, 0};
  channelsMAC[0] = frequencies_table[mac % 25];
  
  channelsMAC[1] = frequencies_table[(mac * 3) % 25];
  int i = 1;
  while (abs(channelsMAC[1] - channelsMAC[0]) <= 400000) {
    channelsMAC[1] = frequencies_table[(mac * 3 + i) % 25]; 
    i++;
  }
  
  channelsMAC[2] = frequencies_table[(mac * 7) % 25];  
  i = 1;
  while (abs(channelsMAC[2] - channelsMAC[0]) <= 400000 || abs(channelsMAC[2] - channelsMAC[1]) <= 400000) {
    channelsMAC[2] = frequencies_table[(mac * 7 + i) % 25];
    i++;
  }

  return channelsMAC;
}

#if defined(ARDUINO_ARCH_STM32)
/**
 * Retrieve unique device identifier (UID) for STM32 chip
 */
uint64_t getUniqueMAC() 
{
    uint32_t UID[3];
    UID[0] = *(uint32_t *)0x1FFF7A10;
    UID[1] = *(uint32_t *)0x1FFF7A14;
    UID[2] = *(uint32_t *)0x1FFF7A18;
    return ((uint64_t)(UID[1] & 0xFFFF) << 32) | UID[0];
}
#endif

void setup() {  
  #ifdef OLED_ON
    Wire.begin();
    display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false);
  #endif  
  
#if defined(ARDUINO_ARCH_STM32)
  pinMode(ADF7023_SPI_CS, OUTPUT);
  digitalWrite(ADF7023_SPI_CS, HIGH);

  // Initialize SPI interface for STM32
  adf7023SPI.setMOSI(ADF7023_SPI_MOSI);
  adf7023SPI.setMISO(ADF7023_SPI_MISO);
  adf7023SPI.setSCLK(ADF7023_SPI_SCLK);
  adf7023SPI.begin();

  chipId = getUniqueMAC();
#else
  chipId = ESP.getEfuseMac();
#endif
  aodv.setMAC(chipId);
    
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize communication with XBee interface
  aodv.xbee.begin(Serial, aodv.xbee._baudrate);

  #ifdef OLED_ON
    display.clearDisplay();
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0,0);
    display.print(aodv.macToStr(aodv.MAC_ID));
    display.display();
  #endif  
  
  // Calculate RX frequency table for the current node
  long* channelsMAC = calcFrequenciesByMAC(chipId);
  for(int i = 0; i < 3; i++) rx_frequencies[i] = channelsMAC[i];
  rx_frequencies[3] = FREQ_BROADCAST_RREQ;
  rx_frequencies[4] = FREQ_RREP;
  
  // Initialize the ADF7023 radio module
  int init = -1; 
  while (init == -1){
    init = ADF7023.init(rx_frequencies[0], 0x80, 81920, 50000);
    if (init != -1) 
    {
      ADF7023.set_fw_state(FW_STATE_PHY_ON);
    } else {
      ADF7023.set_fw_state(FW_STATE_HW_RESET);
      ADF7023.remove();
    }
  }

  pinMode(RF_IRQ_PIN, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(RF_IRQ_PIN), rf_irq, RISING);

  ADF7023.set_fw_state(FW_STATE_PHY_RX);
  delayMicroseconds(200);

  uint8_t clear_irq = 0xFF;
  ADF7023.set_ram(MCR_REG_INTERRUPT_SOURCE_0, 0x1, &clear_irq);

  packetFound = false;

  #ifdef OLED_ON
    display.setCursor(0,40);
    display.print("Success");
    display.display();
  #endif  

}

void freq_swap(long ch){
  ADF7023.set_channel_frequency(ch);
}

void transmit(AODVPacket &packet, long* tx_frequency);

/**
 * Handle routing and transmit the compiled AODV packet
 */
void transmitPacket(AODVPacket &packet){
  if (packet.packet.type != NULL_PACKET_TYPE) {
      if (packet.packet.type == RREQ_PACKET_TYPE && packet.packet.sourceMAC != aodv.MAC_ID){
        ledST = true;
        digitalWrite(LED_PIN, HIGH);
        ledTimer = millis();
      }
      long tx_frequencies[3];
      if (packet.packet.receiverMAC == 0) { 
        tx_frequencies[0] = FREQ_BROADCAST_RREQ;
        tx_frequencies[1] = FREQ_RREP;
        tx_frequencies[2] = FREQ_BROADCAST_RREQ; 
      } else {
        long* channelsMAC = calcFrequenciesByMAC(packet.packet.receiverMAC);
        tx_frequencies[0] = channelsMAC[0];
        tx_frequencies[1] = channelsMAC[1];
        tx_frequencies[2] = channelsMAC[2]; 
     } 
     transmit(packet, tx_frequencies);
  }
}

/**
 * Physical data transmission using Carrier-Sense Multiple Access (CSMA-CA)
 */
void transmit(AODVPacket &packet, long* tx_frequency){
  bool chkRSSI = true;
  int chk_tx_freq = 0;
  int csma_timeout = 0;
  int selected_tx_freq = 0;

  // Listen-Before-Talk (LBT) RSSI checks before initiating transmission
  while(chkRSSI && csma_timeout < 100)
  {
      chkRSSI = false;
      ADF7023.set_fw_state(FW_STATE_PHY_ON);

      if (packet.packet.receiverMAC == 0) 
      {
        ADF7023.set_channel_frequency(tx_frequency[chk_tx_freq]);
        selected_tx_freq = chk_tx_freq;
        chk_tx_freq++;
        if (chk_tx_freq >= 2) chk_tx_freq = 0;
      } 
      else 
      {
        ADF7023.set_channel_frequency(tx_frequency[chk_tx_freq]);
        selected_tx_freq = chk_tx_freq;
        chk_tx_freq++;
        if (chk_tx_freq >= 3) chk_tx_freq = 0;
      }

      ADF7023.set_fw_state(FW_STATE_PHY_RX);
      delayMicroseconds(20);
      if (ADF7023.readRSSI_PHY_RX() > RSSI_LIMIT) chkRSSI = true;
      if (chkRSSI){
         delayMicroseconds(random(300, 900));
         csma_timeout++;
      }
  }

  // If the channel is clear, perform transmission
  if(!chkRSSI) {
    uint8_t* raw = packet.generateRawData();
    uint8_t rawLen = packet.rawDataLength();
    ADF7023.transmit_packet(raw, rawLen);
   
    while (ADF7023.get_fw_state() == FW_STATE_PHY_TX){} 
    
    uint8_t interrupt_reg = 0xFF;
    ADF7023.set_ram(MCR_REG_INTERRUPT_SOURCE_0, 0x1, &interrupt_reg);
  } else {
    ADF7023.set_fw_state(FW_STATE_PHY_ON);
  }
  
  // Revert back to active scanning frequency
  ADF7023.set_channel_frequency(rx_frequencies[channelSwitchCounter]);
  ADF7023.set_fw_state(FW_STATE_PHY_RX);
  uint8_t clear_irq = 0xFF;
  ADF7023.set_ram(MCR_REG_INTERRUPT_SOURCE_0, 0x1, &clear_irq);
  packetFound = false;
}

uint8_t readIrq0() {
  uint8_t irq = 0;
  ADF7023.get_ram(MCR_REG_INTERRUPT_SOURCE_0, 0x1, &irq);
  return irq;
}

/**
 * Switch scanning frequency channel (Frequency Hopping)
 */
void swapfreq(bool forced = false){
  if (FREQ_SWAP_DELAY <= (unsigned long)(micros() - listenTimer) || forced){
        channelSwitchCounter++;
        if (channelSwitchCounter >= CHANNELS) channelSwitchCounter = 0;
      
        ADF7023.set_fw_state(FW_STATE_PHY_ON);
        freq_swap(rx_frequencies[channelSwitchCounter]);
        uint8_t clear_irq = 0xFF;
        ADF7023.set_ram(MCR_REG_INTERRUPT_SOURCE_0, 0x1, &clear_irq);
        packetFound = false;
        ADF7023.set_fw_state(FW_STATE_PHY_RX);
        listenTimer = micros();
    }
}

void loop() {
  // Check the packet detection flag set by transceiver ISR
  if (packetFound){
    packetFound = false;
    
    if(ADF7023.readRSSI_PHY_RX() > RSSI_LIMIT){
      unsigned long timeOut = micros();
       if (ADF7023.preambleDetected()) {
		   
        timeOut = micros();
        
        while(ADF7023.available() == 0) {
          if ((unsigned long)(30000) < (unsigned long)(micros() - timeOut)) break;
        }
      
        if (ADF7023.available()) {
          ADF7023.receive_packet(rx_packet, &len);
          float rssi = ADF7023.readRSSI_PHY_ON();
         
          // Pass the received packet payload to AODV routing library
          AODVPacket aodvPacket = aodv.receive(rx_packet, len, rssi);
          if (aodvPacket.packet.type != NULL_PACKET_TYPE){
            ledST = true;
            digitalWrite(LED_PIN, HIGH);
            ledTimer = millis();
          }
        }
      }
    } else {
      swapfreq(true);
    }
    uint8_t clear_irq = 0xFF;
    ADF7023.set_ram(MCR_REG_INTERRUPT_SOURCE_0, 0x1, &clear_irq);
  } else {
    // Hop frequencies and process background routing tasks
    swapfreq();
    
    int rowID = aodv.queueProcess();
    if (rowID > -1) {
      AODVPacket aodvPacket = aodv.transmit(rowID); 
      transmitPacket(aodvPacket);     
    }
    
    aodv.xbeeProcess(); 
  
  }

  // Reset the LED indicator after timeout delay
  if (LED_DELAY < (unsigned long)(millis() - ledTimer)){
    if (ledST){
      digitalWrite(LED_PIN, LOW);
      ledST = false;
    }
  }
}
