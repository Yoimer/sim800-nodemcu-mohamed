
/* receives and processes sms based on predefined strings,
  including a password in order to take action control(change led or relay state),
  registering and deleting users.
  ONLY the first 5 numbers on sim will be able to
  keep adding and deleting users.
  ONLY the registered users in the "whitelist"
  will be able to tale action control.
  ANY sms DIFFERENT that the predefined strings
  will be IGNORED by the system 

  in order to activate relay or led the sms has to be:
  LED ON,7777, if key saved on position 1 was 7777 
  (only will process if and only if phone sender was previously saved)

  in order to deactivate relay or led sms has to be
  LED OFF,7777, if key saved on position 1 was 7777
  (only will process if and only if phone sender was previously saved)

  in order to add a number in system:
  ADD,position in sim,number to save,
  ADD,3,04168262668,

  in order to delete a number
  DEL,position in sim,
  DEL,3, (will delete user on position 3 in sim)

  to temperature monitoring the sms is:
  TEMP?

  */

int8_t answer;
bool isIncontact                          = false;
bool isAuthorized                         = false;
int x;
unsigned long xprevious;
char currentLine[500]                     = "";
int currentLineIndex                      = 0;
bool nextLineIsMessage                    = false;
bool nextValidLineIsCall                  = false;
String PhoneCallingIndex                  = "";
String PhoneCalling                       = "";
String OldPhoneCalling                    = "";
String lastLine                           = "";
String phonenum                           = "";
int firstComma                            = -1;
int prende                                =  0;
int secondComma                           = -1;
String Password                           = "";
String indexAndName                       = "";
String newContact                         = "";
String trama                              = "";
String temperatureString                  = "";
String humidityString                     = "";
String BuildString                        = "";
String id                                 = "";
String tmpx                               = "";
int SMSerror                              = -1;
int thirdComma                            = -1;
int forthComma                            = -1;
int fifthComma                            = -1;
int firstQuote                            = -1;
int secondQuote                           = -1;
int swveces                               = 0;
int len                                   = -1;
int j                                     = -1;
int i                                     = -1;
int f                                     = -1;
int r                                     = 0;
char number_to_ubidot [25];
bool isInPhonebook = false;
char contact[13];
char phone[21];
char message[100];
char aux_string[100];

// DHT11 sensor libraries

#include "DHT.h"        // including the library of DHT11 temperature and humidity sensor
#define DHTTYPE DHT11   // DHT 11

#define dht_dpin 0
DHT dht(dht_dpin, DHTTYPE); 

#define WIFISSID "SKY7D85E" // Put here your Wi-Fi SSID
#define PASSWORD "msa8121985" // Put her

// NODEMCU PIN OUT
// static const uint8_t D0   = 16;
// static const uint8_t D1   = 5;
// static const uint8_t D2   = 4;
// static const uint8_t D3   = 0;
// static const uint8_t D4   = 2;
// static const uint8_t D5   = 14;
// static const uint8_t D6   = 12;
// static const uint8_t D7   = 13;
// static const uint8_t D8   = 15;
// static const uint8_t D9   = 3;
// static const uint8_t D10  = 1;


// ubidots
#include "UbidotsMicroESP8266.h"
#define TOKEN  "A1E-MAyHKfeWs9oAvAIYd4zLX9cMaey1oj"  // Put here your Ubidots TOKEN
Ubidots client(TOKEN);

// initial setup

void setup() {
  
  // start dht11 sensor
  dht.begin();

  // built-in led as output
  pinMode(LED_BUILTIN, OUTPUT);

  // D2 as output. D2 is GPIO-4
  pinMode(4, OUTPUT);

  // D4 as output. D2 is GPIO-2
  pinMode(2, OUTPUT);


  Serial.begin(115200);

  client.wifiConnection(WIFISSID, PASSWORD);

  Serial.println("Starting...");
//checking physical connection from NODEMCU to simn800l
  power_on(); 
  delay(3000);
  Serial.println("Connecting to the network...");
  
  // checking cellular connection
  while ( (sendATcommand("AT+CREG?", "+CREG: 0,1", 5000, 0) ||
           sendATcommand("AT+CREG?", "+CREG: 0,5", 5000, 0)) == 0 );

  // configuring sms reading as ASCII characters
  sendATcommand("AT+CMGF=1", "OK", 5000, 0);

  // configuring sms reading just in serial buffer NOT saving messages on simcard
  sendATcommand("AT+CNMI=1,2,0,0,0", "OK", 5000, 0);
  
  // reading first contact previously saved on sim
  // this is master key (password) to be used in sms queries
  sendATcommand("AT+CPBR=1,1", "OK\r\n", 5000, 1);
  
  Serial.println("Password:");

  // printing password on console
  Serial.println(Password);
  

}

//**********************************************************


// measure temperature
float getTemperature() {
    float temp = dht.readTemperature();
    Serial.print("temperature = ");
    Serial.print(temp); 
    Serial.println("C  ");
    return temp;
}

// measure humidity
float getHumidity() {
    float humidity = dht.readHumidity();
    Serial.print("Current humidity = ");
    Serial.print(humidity);
    Serial.print("%  ");
    return humidity;
}


//**********************************************************


// main program

void loop()
{

  unsigned long previous = millis();

  do
  {
    //if there is serial activity
    if (Serial.available() > 0)
    {
    char lastCharRead = Serial.read();
    
    // read every char until end of line
    if (lastCharRead == '\r' || lastCharRead == '\n')
    {
      endOfLineReached();
    }

    else
    {
      currentLine[currentLineIndex++] = lastCharRead;
    }
      }
  }while((millis() - previous) < 5000);  // check for serial activity each 5 seconds
}
//**********************************************************

// function that checks physical connection from NODEMCU to sim800l
void power_on()
{
  uint8_t answer = 0;
  answer = sendATcommand("AT", "OK", 5000, 0);
  if (answer == 0)
  {
    while (answer == 0)
    {
      answer = sendATcommand("AT", "OK", 2000, 0);
    }
  }
}

//**********************************************************

// function that sends AT commands to sim800l
// when int xpassword has 0 it does not check password
// when int xpassword has 1 it does check password

int8_t sendATcommand(char* ATcommand, char* expected_answer, unsigned int timeout, int xpassword)
{
  uint8_t x = 0,  answer = 0;
  char response[100];
  unsigned long previous;
  memset(response, '\0', 100);
  delay(100);
  while (Serial.available())
  {
    Serial.read();
  }
  Serial.println(ATcommand);
  x = 0;
  previous = millis();
  do
  {
    if (Serial.available() != 0)
    {
      response[x] = Serial.read();
      x++;
      if (strstr(response, expected_answer) != NULL)
      {
        answer = 1;
        String numbFromSim = String(response);
        numbFromSim = numbFromSim.substring(numbFromSim.indexOf(":"),
                                            numbFromSim.indexOf(",129,"));
        numbFromSim = numbFromSim.substring((numbFromSim.indexOf(34) + 1),
                                            numbFromSim.indexOf(34, numbFromSim.indexOf(34) + 1));
        if ( xpassword == 1)
        {
          numbFromSim = numbFromSim.substring( 0, 4);
          Password = numbFromSim ;
          return 0;
        }
        else
        {
          numbFromSim = numbFromSim.substring( 0, 11 );
        }
      }
    }

  }
  while ((answer == 0) && ((millis() - previous) < timeout));
  return answer;
}

//**********************************************************

// function that reads end of line in serial port
void endOfLineReached()

{
  lastLine = String(currentLine);
// checks that system is receiving a phone call
  if (lastLine.startsWith("RING"))
  {
    Serial.println(lastLine);
    nextValidLineIsCall = true;
  }
  else
  {
    if ((lastLine.length() > 0) && (nextValidLineIsCall))
    {
      // process call
      //LastLineIsCLIP();
    }

    // checks that system is receiving sms
    else if (lastLine.startsWith("+CMT:"))
    {

    Serial.println(lastLine);

    // extracts phone number   
      phonenum = lastLine.substring((lastLine.indexOf(34) + 1),
                                    lastLine.indexOf(34, lastLine.indexOf(34) + 1));

      String temporal = "num=" + phonenum;
      temporal.toCharArray(number_to_ubidot, 25);
      Serial.println("to ubidot");
      Serial.println(number_to_ubidot);
      nextLineIsMessage = true;
      firstComma        = lastLine.indexOf(',');
      secondComma       = lastLine.indexOf(',', firstComma  + 1);
      thirdComma        = lastLine.indexOf(',', secondComma + 1);
      forthComma        = lastLine.indexOf(',', thirdComma  + 1);
      fifthComma        = lastLine.indexOf(',', forthComma  + 1);
      PhoneCalling      = lastLine.substring((firstComma - 12), (firstComma - 1));
      PhoneCallingIndex = lastLine.substring((firstComma + 2), (secondComma - 1));
      Serial.println(phonenum);        ////////////////////////////////////////////
      Serial.println(PhoneCallingIndex);
      j            = PhoneCallingIndex.toInt();
      isIncontact  = false;
      isAuthorized = false;
    
    // validates register and authorization
      if (j > 0)
      {
        isIncontact = true;
        Serial.println("in phonebook");
        if (j <= 5 )
        {
          Serial.println("authorized");
          isAuthorized = true;
        }
      }
      else
      {
      }
    }
    else if ((lastLine.length() > 0) && (nextLineIsMessage))
    {
    // processes sms
    LastLineIsCMT();
    }
  }
  // cleans buffer
  CleanCurrentLine();
}

//**********************************************************

// function that cleans line
void CleanCurrentLine()

{
  for ( int i = 0; i < sizeof(currentLine);  ++i )
  {
    currentLine[i] = (char)0;
  }
  currentLineIndex = 0;
}

//**********************************************************

// function that processes sms

void LastLineIsCMT()
{

  // only sms content
  Serial.println(lastLine);

  // cleans buffer
  clearBuffer();

  // just from position 1 to 5 in sim
  if (isAuthorized)
  {
      //SMS to turn LED ON
    if (lastLine.indexOf("LED ON") >= 0)
    {
      // inverse logic(negative logic)
      prendeapaga(0);
    }

    // sms to turn LED OFF
    else if (lastLine.indexOf("LED OFF") >= 0)
    {
      // inverse logic(negative logic)
      prendeapaga(1);
    }

    else if (lastLine.indexOf("LED1 ON") >= 0)
    {
      // inverse logic(negative logic)
      prendeapaga1(0);
    }

    else if (lastLine.indexOf("LED1 OFF") >= 0)
    {
      // inverse logic(negative logic)
      prendeapaga1(1);
    }
      // sms to add user
    else if (lastLine.indexOf("ADD") >= 0)
    {
      DelAdd(1);
    }

    // sms to delete user
    else if (lastLine.indexOf("DEL") >= 0)
    {
      DelAdd(2);
    }
    else if (lastLine.indexOf("TEMP?") >= 0) 
    {
      getTemperatureSMS();
    }
    else if (lastLine.indexOf("HUMD?") >= 0)
    {
      getHumiditySMS();
    }
    else
    {
      clearBuffer();
    }
  } 
  // any registered can ask for temperature and humidity
  else if (isIncontact) 
  {
    if (lastLine.indexOf("TEMP?") >= 0) 
    {
      getTemperatureSMS();
    }
    else if (lastLine.indexOf("HUMD?") >= 0)
    {
      getHumiditySMS();
    }
    else 
    {
      clearBuffer();
    }
  }

  CleanCurrentLine();
  nextLineIsMessage = false;
}

//**********************************************************

// function that confirms password present in sms body

int  prendeapaga (int siono)
{
  Serial.println("KKKKKKKKKKKKKKKKKKKKKKKKKKK");
  Serial.println(lastLine);
  firstComma    = lastLine.indexOf(',');
  secondComma   = lastLine.indexOf(',', firstComma  + 1);
  String InPassword = lastLine.substring((firstComma + 1), (secondComma));
  Serial.println(InPassword);

  if (InPassword == Password)
  {
// nodemcu led with inverse logic (negative logic)
    digitalWrite(LED_BUILTIN, siono);
  // led connected in digital port D2-GPIO-4
    switch (siono)
    {
      case 0:
        // activates led with inverse logic (negative logic)
        digitalWrite(4, LOW);

        // copy number in array phone
        phonenum.toCharArray(phone, 21);
      
        // sends sms confirmation
        sendSMS(phone, "LED is ON!");
        client.add("LED",1,number_to_ubidot);
        client.sendAll(true);
        break;
      case 1:

        // deactivate led with inverse logic (negative logic)
        digitalWrite(4, HIGH);
      
        // copy number in array phone
        phonenum.toCharArray(phone, 21);
      
        // sends sms confirmation
        sendSMS(phone, "LED is OFF!");
        client.add("LED",0,number_to_ubidot);
        client.sendAll(true);
        break;
      default:
      break;
    }
  }
  // cleans buffer
  clearBuffer();
}


//**********************************************************

// function that confirms password present in sms body
// for second led

int  prendeapaga1 (int siono)
{
  Serial.println("KKKKKKKKKKKKKKKKKKKKKKKKKKK");
  Serial.println(lastLine);
  firstComma    = lastLine.indexOf(',');
  secondComma   = lastLine.indexOf(',', firstComma  + 1);
  String InPassword = lastLine.substring((firstComma + 1), (secondComma));
  Serial.println(InPassword);

  if (InPassword == Password)
  {

    // nodemcu led with inverse logic (negative logic)
    digitalWrite(LED_BUILTIN, siono);
  // led connected in digital port D2-GPIO-4
    switch (siono)
    {
      case 0:
        // activates led with inverse logic (negative logic)
        digitalWrite(2, LOW);

        // copy number in array phone
        phonenum.toCharArray(phone, 21);
      
        // sends sms confirmation
        sendSMS(phone, "LED1 is ON!");
        client.add("LED1",1,number_to_ubidot);
        client.sendAll(true);

        break;
      case 1:

        // deactivate led with inverse logic (negative logic)
        digitalWrite(2, HIGH);
      
        // copy number in array phone
        phonenum.toCharArray(phone, 21);
      
        // sends sms confirmation
        sendSMS(phone, "LED1 is OFF!");
        client.add("LED1",0,number_to_ubidot);
        client.sendAll(true);
        break;
      default:
      break;
    }
  }
  // cleans buffer
  clearBuffer();
}

//**********************************************************

// function that cleans buffer
void clearBuffer()
{
  byte w = 0;
  for (int i = 0; i < 10; i++)
  {
    while (Serial.available() > 0)
    {
      char k = Serial.read();
      w++;
      delay(1);
    }
    delay(1);
  }
}

//**********************************************************

// function that register and delete users

int DelAdd(int DelOrAdd)
{
  indexAndName = "";
  ////char aux_string[100];
  firstComma          = lastLine.indexOf(',');
  secondComma         = lastLine.indexOf(',', firstComma  + 1);
  thirdComma          = lastLine.indexOf(',', secondComma + 1);
  indexAndName = lastLine.substring((firstComma + 1), (secondComma));
  newContact = "";
  newContact   = lastLine.substring((secondComma + 1), thirdComma);
  
  // if registered on sim
  if (!isAuthorized)
  {
    Serial.println(j);
    Serial.println("Not authorized to Delete/Add");
    return 0;
  }
  
  // cleans tmpx
  tmpx = "";
  
  // Comando AT para agregar y borrar usuarios en el SIM
  // at commands for adding and deleting

    // add
    //AT+CPBW=sim position,"number to be saved",129,"contact name"
    //AT+CPBW=1,"04168262665",129,"1"
  
  // delete
  // AT+CPBW=posicion on sim
  // AT+CPBW=30 // would delete number in position 30
tmpx = "AT+CPBW=" + indexAndName + "\r\n\"";
  if ( DelOrAdd == 1 )
  {
    tmpx = "AT+CPBW=" + indexAndName + ",\"" + newContact + "\"" + ",129," + "\"" + indexAndName + "\"" + "\r\n\"";
  }
  tmpx.toCharArray( aux_string, 100 );
  Serial.println(aux_string);
  answer = sendATcommand(aux_string, "OK", 20000, 0);
  if (answer == 1)
  {
    Serial.println("Sent ");
    confirmSMS(DelOrAdd);
  }
  else
  {
    Serial.println("error ");
  
  // Error de registro
  if (DelOrAdd == 1)
  {
    SMSerror = 1;
  }
  // Error de eliminación
    else if (DelOrAdd == 2)
  {
    SMSerror = 2;
  }
  Serial.println("Go to error routine");
  // llamar a confirmSMS() para decir que tipo de error hubo
  confirmSMS(3);
  }
  clearBuffer();
}


//**********************************************************

// function than send sms

int sendSMS(char *phone_number, char *sms_text)
{
  char aux_string[30];
  //char phone_number[] = "04168262667"; // ********* is the number to call
  //char sms_text[] = "Test-Arduino-Hello World";
  Serial.print("Setting SMS mode...");
  sendATcommand("AT+CMGF=1", "OK", 5000, 0);   // sets the SMS mode to text
  Serial.println("Sending SMS");

  sprintf(aux_string, "AT+CMGS=\"%s\"", phone_number);
  answer = sendATcommand(aux_string, ">", 20000, 0);   // send the SMS number
  if (answer == 1)
  {
    Serial.println(sms_text);
    Serial.write(0x1A);
    answer = sendATcommand("", "OK", 20000, 0);
    if (answer == 1)
    {
      Serial.println("Sent ");
    }
    else
    {
      Serial.println("error ");
    }
  }
  else
  {
    Serial.println("error ");
    Serial.println(answer, DEC);
  }
  return answer;
}

//**********************************************************

// function that confirms registration, deleting
// and errorv via sms

void confirmSMS(int DelOrAdd )
{
  switch (DelOrAdd) {

    // registering confirmation
    case 1:
      trama = "";
      trama = "Number: " + newContact + " has been registered successfully: " + indexAndName;

      tramaSMS(phonenum, trama); // SMS confirmation

      trama = "";
      trama = "Welcome. Your number has been registered successfully";
      tramaSMS(newContact, trama); //SMS confirmation
      break;

    // successfully deleting
    case 2:
      trama = "";
      trama = "Number registered in position: " + indexAndName + " has been changed successfully";
      tramaSMS(phonenum, trama); // SMS confirmation
      break;
    // error report
    case 3:
      Serial.println("On case 3 ");
      Serial.println("Value of DelOrAdd: ");
      Serial.println(DelOrAdd);
      switch (SMSerror) {
        case 1:
          trama = "";
          trama = "Number could not be registered. Check message format please";
          tramaSMS(phonenum, trama); // SMS confirmation
          break;
        // deleting error
        case 2:
          trama = "";
          trama = "Number could not be deleted. Check message format please";
          tramaSMS(phonenum, trama); // SMS confirmation
          break;
        default:
        break;
      }
      break;
    default:
    break;
  }
}

//**********************************************************

// build string to be delivered as a sms

void tramaSMS(String numbertoSend, String messagetoSend)
{
  // copy number in arrays
  strcpy(phone,numbertoSend.c_str());

  // trama into message
  strcpy(message, messagetoSend.c_str());

  // sms confirmation
  sendSMS(phone, message);
}

//**********************************************************

// function that gets temperature from DHT11 and sends sms

void getTemperatureSMS()
{   
    // checks if number is whithin 5 first positions in sim
    // measure temperature
    float temperature = getTemperature();
    temperatureString = "";
    temperatureString = String(getTemperature());
    // prints temperature in console
    // Serial.println(temperatureString);
    trama = "";
    trama = "Temperature value is: " + temperatureString + " Celsius degrees";
    tramaSMS(phonenum, trama);

    // client.add("query",1 ,number_to_ubidot);  // we added the request at the end  as context
    //client.add("humidity", value, context);
    client.add("temperature",temperature,number_to_ubidot);
    client.sendAll(true);

}

//**********************************************************
// function that gets humidity from DHT11 and sends sms
void getHumiditySMS()
{
      // checks if number is whithin 5 first positions in sim
    // measure humidity
    float humidity = getHumidity();
    humidityString = "";
    humidityString = String(getHumidity());
    // prints humidity in console
    // Serial.println(humidityString);
    trama = "";
    trama = "Humidity value is: " + humidityString + " Percentage";
    tramaSMS(phonenum, trama);

    client.add("humidity",humidity,number_to_ubidot);
    client.sendAll(true);

  
}
