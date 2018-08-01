/*
  receives and processes sms based on predefined strings,
  including a password in order to take action control(change led or relay state),
  registering and deleting users.
  ONLY the first 5 numbers on sim will be able to
  keep adding and deleting users.
  ONLY the registered users in the "whitelist"
  will be able to tale action control.
  ANY sms DIFFERENT that the predefined strings
  will be IGNORED by the system

  Warning: when running this sketch, all the saved contacts on sim will be WIPED OUT!

  sms format:
  KEY,4 numbers as password,
  for example the number 04168262661 sends the sms KEY,7777,
  on position 1 in sim is saved the key 7777
  on position 2 in sim the 04168262661 is saved
  from now only the number 04168262661 is able to add people using the 7777 password.


*/

int8_t answer;
bool isIncontact                              = false;
bool isAuthorized                             = false;
int x;
unsigned long xprevious;
char currentLine[500]                         = "";
int currentLineIndex                          = 0;
bool nextLineIsMessage                        = false;
bool nextValidLineIsCall                      = false;
String PhoneCallingIndex                      = "";
String PhoneCalling                           = "";
String OldPhoneCalling                        = "";
String lastLine                               = "";
String phonenum                               = "";
String indexAndName                           = "";
String tmpx                                   = "";
String trama                                  = "";
int firstComma                                = -1;
int prende                                    = 0;
int secondComma                               = -1;
String Password                               = "";
int thirdComma                                = -1;
int forthComma                                = -1;
int fifthComma                                = -1;
int firstQuote                                = -1;
int secondQuote                               = -1;
int swveces                                   = 0;
int len                                       = -1;
int j                                         = -1;
int i                                         = -1;
int f                                         = -1;
int r                                         = 0;
bool isInPhonebook                            = false;
char contact[13];
char phone[21];
char message[100];

//**********************************************************

// initial setup

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
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
  
}


//**********************************************************

// main program
void loop()
{
  CheckSIM800L();
}

//**********************************************************

// function that checks physical connection from NODEMCU to simn800l
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
      //LastLineIsCLIP();
    }

  // checks that system is receiving sms
    else if (lastLine.startsWith("+CMT:"))                         
    {

    // prints full received sms
    // includes sender number
    Serial.println(lastLine);

    // extracts phone number
      phonenum = lastLine.substring((lastLine.indexOf(34) + 1),
                                    lastLine.indexOf(34, lastLine.indexOf(34) + 1));
      nextLineIsMessage = true;
      firstComma        = lastLine.indexOf(',');
      secondComma       = lastLine.indexOf(',', firstComma  + 1);
      thirdComma        = lastLine.indexOf(',', secondComma + 1);
      forthComma        = lastLine.indexOf(',', thirdComma  + 1);
      fifthComma        = lastLine.indexOf(',', forthComma  + 1);
      //PhoneCalling      = lastLine.substring((firstComma - 12), (firstComma - 1));
      PhoneCallingIndex = lastLine.substring((firstComma + 2), (secondComma - 1));
      Serial.println(phonenum);
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
  
  // extract password
  // sms example having KEY,0007, as content

  firstComma        = lastLine.indexOf(',');
  secondComma       = lastLine.indexOf(',', (firstComma + 1));
  String key        = lastLine.substring((firstComma + 1), (secondComma));
  Serial.println(key);

  clearBuffer();
  
    // sms to insert password
  if (lastLine.indexOf("KEY") >= 0)
    {
		deleteAllContacts();
		addContact("2",phonenum);
		addContact("1",key);
		trama = "";
		// trama = "Su numero ha sido registrado exitosamente y la clave principal es: " + key;
	  trama = "phone number has been registered successfully and main password is: " + key;
		tramaSMS(phonenum, trama);
    }
    else
    {
      clearBuffer();
    }
  CleanCurrentLine();
  nextLineIsMessage = false;
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

// function that wipes out all contacts in sim

void deleteAllContacts()
{

	char aux_string[100];
  // for a simcard of 250 contacts
  // change number if needed
	for (i = 1; i < 251; i++)
	{
		indexAndName = "";
		indexAndName = String(i);
		tmpx = "";
		tmpx = "AT+CPBW=" + indexAndName + "\r\n\"";
		tmpx.toCharArray( aux_string, 100 );
		Serial.println(aux_string);
		answer = sendATcommand(aux_string, "OK", 20000, 0); // sends At command
		if (answer == 1)
		{
			Serial.println("Eliminado ");
		}
		else
		{
			Serial.println("error ");
		}
	}
}

//**********************************************************

// function that adds phone number in predefined position in position variable

void addContact(String position, String number)
{
	char aux_string[100];
	tmpx = "AT+CPBW=" + position + ",\"" + number + "\"" + ",129," + "\"" + position + "\"" + "\r\n\"";
	//Serial.println(tmpx);
	tmpx.toCharArray( aux_string, 100 );
	//Serial.println(aux_string);
	answer = sendATcommand(aux_string, "OK", 20000, 0); // sends At command
	if (answer == 1)
	{
		Serial.println("Agregado ");
		}
		else
		{
			Serial.println("error ");
		}
}

//**********************************************************

// functions that builds message to be later delivered as a sms

void tramaSMS(String numbertoSend, String messagetoSend)
{
  // copies number in array phone
	strcpy(phone,numbertoSend.c_str());

  // converts message in sms
	strcpy(message, messagetoSend.c_str());

  // sends sms confirmation
	sendSMS(phone, message);
}

//**********************************************************
// function that sends sms

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
// function that checks sim800l

void CheckSIM800L()
{
  if (Serial.available() > 0)
  {
    char lastCharRead = Serial.read();
    if (lastCharRead == '\r' || lastCharRead == '\n')
    {
      endOfLineReached();
    }
    else
    {
      currentLine[currentLineIndex++] = lastCharRead;
    }
  }
}