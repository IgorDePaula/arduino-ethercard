#include <EtherCard.h>

#define STATIC 0  // set to 1 to disable DHCP (adjust myip/gwip values below)
#define Rele1 3 // Define pino de saida para rele 1
#define Rele2 5 // Define pino de saida para rele 2
#if STATIC
// ethernet interface ip address
static byte myip[] = { 
  192,168,1,200 };
// gateway ip address
static byte gwip[] = { 
  192,168,0,1 };
#endif

// ethernet mac address - must be unique on your network
static byte mymac[] = { 
  0x74,0x69,0x69,0x2D,0x30,0x31 };
#define LED 6  // define LED pin
bool ledStatus = false;
bool ledStatus2= false;

byte Ethernet::buffer[500]; // tcp/ip send and receive buffer
BufferFiller bfill;

void setup(){
  Serial.begin(57600);
  Serial.println("\n[backSoon]");
pinMode(Rele1,OUTPUT);
pinMode(Rele2,OUTPUT);
  if (ether.begin(sizeof Ethernet::buffer, mymac) == 0) 
    Serial.println( "Failed to access Ethernet controller");
#if STATIC
  ether.staticSetup(myip, gwip);
#else
  if (!ether.dhcpSetup())
    Serial.println("DHCP failed");
#endif

  ether.printIp("IP:  ", ether.myip);
  ether.printIp("GW:  ", ether.gwip);  
  ether.printIp("DNS: ", ether.dnsip);  
}
const char http_OK[] PROGMEM =
"HTTP/1.0 200 OK\r\n"
"Content-Type: text/html\r\n"
"Pragma: no-cache\r\n\r\n";

const char http_Found[] PROGMEM =
"HTTP/1.0 302 Found\r\n"
"Location: /\r\n\r\n";

const char http_Unauthorized[] PROGMEM =
"HTTP/1.0 401 Unauthorized\r\n"
"Content-Type: text/html\r\n\r\n"
"<h1>401 Unauthorized</h1>";

void homePage()
{
  bfill.emit_p(PSTR("$F"
    "<meta http-equiv='refresh' content='5'/>"
    "<title>Ethercard LED</title>" 
    "ledStatus: <a href=\"?led=$F\">$F</a><br>ledStatus2: <a href=\"?led2=$F\">$F</a>"),
  http_OK,
  ledStatus?PSTR("off"):PSTR("on"),
  ledStatus?PSTR("ON"):PSTR("OFF"),
  ledStatus2?PSTR("off"):PSTR("on"),
  ledStatus2?PSTR("ON"):PSTR("OFF"));
}

void loop(){
  // DHCP expiration is a bit brutal, because all other ethernet activity and
  // incoming packets will be ignored until a new lease has been acquired
//  if (!STATIC && ether.dhcpExpired()) {
//    Serial.println("Acquiring DHCP lease again");
//    ether.dhcpSetup();
//  }
  digitalWrite(LED, !ledStatus); 
  Serial.print(!ledStatus);
  Serial.print(!ledStatus2);
  Serial.println("");
  // wait for an incoming TCP packet, but ignore its contents
  word len = ether.packetReceive();
  word pos = ether.packetLoop(len); 
  if (pos) {
    // write to LED digital output

    delay(1);   // necessary for my system
    bfill = ether.tcpOffset();
    char *data = (char *) Ethernet::buffer + pos;
    if (strncmp("GET /", data, 5) != 0) {
      // Unsupported HTTP request
      // 304 or 501 response would be more appropriate
      bfill.emit_p(http_Unauthorized);
    }
    else {
      data += 5;

      if (data[0] == ' ') {
        // Return home page
        homePage();
      }
      else if (strncmp("?led=off ", data, 9) == 0) {
        ledStatus=false;   
        digitalWrite(Rele1,LOW);
        bfill.emit_p(http_Found);
      }
      else if (strncmp("?led=on ", data, 8) == 0) {

        ledStatus=true;  
        digitalWrite(Rele1, HIGH);


        bfill.emit_p(http_Found);
      }
      else if (strncmp("?led2=off ", data, 10) == 0) {

        ledStatus2=false;  
digitalWrite(Rele2, LOW);

        bfill.emit_p(http_Found);
      }
      else if (strncmp("?led2=on ", data, 9) == 0) {
digitalWrite(Rele2, HIGH);
        ledStatus2=true;  
        bfill.emit_p(http_Found);
      }
      else {
        // Page not found
        bfill.emit_p(http_Unauthorized);
      }
    }

    ether.httpServerReply(bfill.position());    // send http response
  }

}
