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

// wifi libraries
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
ESP8266WiFiMulti WiFiMulti;

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
  
  // connect wifi
  WiFiMulti.addAP("Casa","96525841258");
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
	  // Si hay salida serial desde el SIM800
	  if (Serial.available() > 0)
	  {
		char lastCharRead = Serial.read();
		
		// Lee cada caracter desde la salida serial hasta que \r o \n is encontrado (lo cual denota un fin de línea)
		if (lastCharRead == '\r' || lastCharRead == '\n')
		{
		  endOfLineReached();
		}

		else
		{
		  currentLine[currentLineIndex++] = lastCharRead;
		}
      }
  }while((millis() - previous) < 5000);  // espera actividad en puerto serial for 5 segundos
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
      LastLineIsCLIP();
    }

    // checks that system is receiving sms
    else if (lastLine.startsWith("+CMT:"))
    {

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

  if (isIncontact)
  {
      //SMS to turn LED ON
    if (lastLine.indexOf("LED ON") >= 0)
    {
      // logica inversa
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
    // sms to ask for temperature
    // only 5 first registered numbers on sim may ask for temperature

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
        break;
      case 1:

        // deactivate led with inverse logic (negative logic)
        digitalWrite(4, HIGH);
      
        // copy number in array phone
        phonenum.toCharArray(phone, 21);
      
        // sends sms confirmation
        sendSMS(phone, "LED is OFF!");
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
        break;
      case 1:

        // deactivate led with inverse logic (negative logic)
        digitalWrite(2, HIGH);
      
        // copy number in array phone
        phonenum.toCharArray(phone, 21);
      
        // sends sms confirmation
        sendSMS(phone, "LED1 is OFF!");
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

// Función que procesa llamada telefónica

void LastLineIsCLIP()
{
  firstComma         = lastLine.indexOf(',');
  secondComma        = lastLine.indexOf(',', firstComma + 1);
  thirdComma         = lastLine.indexOf(',', secondComma + 1);
  forthComma         = lastLine.indexOf(',', thirdComma + 1);
  fifthComma         = lastLine.indexOf(',', forthComma + 1);
  PhoneCalling       = lastLine.substring((firstComma - 12), (firstComma - 1));
  PhoneCallingIndex  = lastLine.substring((forthComma + 2), (fifthComma - 1));
  j                  = PhoneCallingIndex.toInt();
  if (PhoneCalling == OldPhoneCalling)
  {
    swveces = 1;
    if ((millis() - xprevious ) > 9000)
    {
      swveces   = 0;
      xprevious = millis();
    };
  }
  else
  {
    xprevious       = millis();
    OldPhoneCalling = PhoneCalling;
    swveces         = 0;
  }
  if (j > 0 & swveces == 0)
  {
    digitalWrite(LED_BUILTIN, HIGH);
	// desactiva el relé con lógica inversa
	digitalWrite(4, HIGH);
  }
  if ((WiFiMulti.run() == WL_CONNECTED) & swveces == 0)
  {
    HTTPClient http;
    String xp = "http://estredoyaqueclub.com.ve/arduinoenviacorreo.php?telefono=" + PhoneCalling + "-" + PhoneCallingIndex;
    http.begin(xp);
    int httpCode = http.GET();
    if (httpCode > 0)
    {
      if (httpCode == HTTP_CODE_OK)
      {
        String BuildStringx = http.getString();
        Serial.println("[+++++++++++++++++++");
        Serial.println(BuildStringx);
        Serial.println("[+++++++++++++++++++");
      }
    }
    else
    {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  }
  clearBuffer();
  nextValidLineIsCall = false;
}



//**********************************************************

// Función que Registra y Borra usuario

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
  
  // confirma que está entre los 5 primeros usuarios del sim
  if (!isAuthorized)
  {
    Serial.println(j);
    Serial.println("Not authorized to Delete/Add");
    return 0;
  }
  
  // Limpia variable temporal
  tmpx = "";
  
  // Comando AT para agregar y borrar usuarios en el SIM
  
  //Agregar
  // AT+CPBW=posicion en sim,"numero a guadar",129,"nombre de contacto"
  // 129 significa que el numero a guardar es nacional
  // sin incluir el formato telefónico internacional ejemplo +58
  // AT+CPBW=1,"04168262667",129,"1" guarda ese número en la
  // posición 1 del sim y el nombre asignado es un ID de valor 1.
  // cada ID está asignado en la base de datos con el nombre
  // del usuario correspondiente
  
  //Borrar
  // AT+CPBW=posicion en sim
  // AT+CPBW=30 borraría posición 30 en el sim

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
	// llamar a confirmSMS() para decir que se registró
	// o se eliminó con éxito
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
	Serial.println("Va a rutina de error");
	// llamar a confirmSMS() para decir que tipo de error hubo
	confirmSMS(3);
  }
  clearBuffer();
}


//**********************************************************

// Función que envía SMS 

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

// Función que confirma registro,eliminación
// o error vía SMS 

void confirmSMS(int DelOrAdd )
{
	switch (DelOrAdd) {
		// Confirma registro exitoso
		case 1:
			trama = "";
			trama = "El numero: " + newContact + " ha sido registrado con exito en la posicion: " + indexAndName;
			tramaSMS(phonenum, trama); // Envía SMS de confirmación 

			trama = "";
			trama = "Bienvenid@. Su numero fue registrado exitosamente.";
			tramaSMS(newContact, trama); // Envía SMS de confirmación 
			break;
		// Confirma eliminación exitosa
		case 2:
			trama = "";
			trama = "El numero registrado en la posicion: " + indexAndName + " ha sido eliminado exitosamente ";
			tramaSMS(phonenum, trama); // Envía SMS de confirmación 
			break;
		// Reporta error
		case 3:
			Serial.println("On case 3 ");
			Serial.println("Value of DelOrAdd: ");
			Serial.println(DelOrAdd);
			switch (SMSerror) {
				// Error de registro
				case 1:
					trama = "";
					trama = "No se pudo registrar el numero. Revise el formato del mensaje por favor";
					tramaSMS(phonenum, trama); // Envía SMS de confirmación 
					break;
				// Error de eliminación
				case 2:
					trama = "";
					trama = "No se pudo eliminar el numero. Revise el formato del mensaje por favor";
					tramaSMS(phonenum, trama); // Envía SMS de confirmación 
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

// Función que arma trama de mensaje para enviar notificación
// via SMS

void tramaSMS(String numbertoSend, String messagetoSend)
{
	// Copia número en array phone
	strcpy(phone,numbertoSend.c_str());

	// Convierte trama en mensaje
	strcpy(message, messagetoSend.c_str());

	// Envía SMS de confirmación 
	sendSMS(phone, message);
}

//**********************************************************

// function that gets temperature from DHT11 and sends sms

void getTemperatureSMS()
{   
    // checks if number is whithin 5 first positions in sim
	if (!isAuthorized)
    {
		Serial.println(j);
		// Serial.println("No autorizado para consultar temperatura");
    Serial.println("No authorized to check temperature");
    }
	else
	{
		// measure temperature
    float temperature = getTemperature();
    temperatureString = "";
    temperatureString = String(getTemperature());
    // prints temperature in console
    // Serial.println(temperatureString);
    trama = "";
    trama = "Temperature value is: " + temperatureString + " Celsius degrees";
		tramaSMS(phonenum, trama);
	}
}

//**********************************************************

// function that gets humidity from DHT11 and sends sms
void getHumiditySMS()
{
      // checks if number is whithin 5 first positions in sim
	if (!isAuthorized)
    {
		Serial.println(j);
		// Serial.println("No autorizado para consultar temperatura");
    Serial.println("No authorized to check humidity");
    }
	else
	{
		// measure humidity
    float humidity = getHumidity();
    humidityString = "";
    humidityString = String(getHumidity());
    // prints humidity in console
    // Serial.println(humidityString);
    trama = "";
    trama = "Humidity value is: " + humidityString + " Percentage";
		tramaSMS(phonenum, trama);
	}
}


//**********************************************************

// Función que se comunica con servidor web y
// verifica comandos de control y registro

int GetInfoFromWeb (int router)
{
//delay(10000);
delay(5000);
String xp;
if((WiFiMulti.run() == WL_CONNECTED) ) 
  {  
	Serial.println("[++++++GetInfoFromWeb+++++++");

  // Servidor web local virtual
  // Debe ser uno real conectado a internet
  //xp = "http://192.168.0.164/sandbox/whitelist.txt";
  //xp = "http://192.168.5.107/sandbox/whitelist.txt";
  //xp = "http://98cc57cb.ngrok.io/sandbox/whitelist.txt";
  xp = "http://192.168.5.102/sandbox/whitelist.txt";
  Serial.println(xp);
  HTTPClient http;
  http.begin(xp);
  int httpCode = http.GET();
  if(httpCode > 0)
  {
  if(httpCode == HTTP_CODE_OK) 
    {
		BuildString = http.getString();
		Serial.println(BuildString);

		// String que viene desde el servidor a modo de espera
		// +9999#99999999999$SMS*AA/position/
   
		// String que viene desde el servidor para tomar acción
		//+9999#99999999999$SMS*2/35/
		//9999                -> ID en base de datos
		//99999999999         -> Celular de 11 dígitos que recibe el mensaje o que va a ser agregado o eliminado del sistema
		//SMS                 -> Contenido del mensaje
		//2                   -> Acción que se toma en el sistema
        //35                  -> Posición en el SIM

		
		//0                   -> Activa Relé por lógica inversa
		//1                   -> Desactiva Relé por lógica inversa
		//2                   -> Agrega número en el SIM
		//3                   -> Borra número en el SIM
		//cada acción debe documentarse acá
		
		char msgx[1024];
		char telx[1024];

		// Extrae ID de la base de datos
		id             = BuildString.substring(BuildString.indexOf("+")+1,BuildString.indexOf("#"));

		// Extrae número telefónico que recibirá el SMS o número que va a ser agregado o eliminado del sistema
		String tel     = BuildString.substring(BuildString.indexOf("#")+1,BuildString.indexOf("$"));
		
		// Extrae SMS
		String msg     = BuildString.substring(BuildString.indexOf("$")+1,BuildString.indexOf("*"));
		
		// Extrae acción a tomar en el sistema
		String action  = BuildString.substring(BuildString.indexOf("*")+1,BuildString.indexOf("/"));
		
		// Extrae position del SIM que será agregada o eliminada del sitema
		String add_del     = BuildString.substring((BuildString.indexOf("/") + 1), BuildString.indexOf("/", BuildString.indexOf("/") + 1));

		Serial.println("id :"+id);
		Serial.println("tel:"+tel);
		Serial.println("msg:"+msg);
		Serial.println("action:"+action);
		Serial.println("add_del:"+add_del);
		
		//Formato de mensaje presente en el servidor
		//+9999#99999999999$SMS*AA/
		if ( action != "AA")
		{
			strcpy(telx, tel.c_str());
			strcpy(msgx, msg.c_str());
			int control = action.toInt();
			Serial.println(control);
			// control de relé desde internet
			// Relé conectado en puerto digital D2-GPIO-4
			switch (control)
			{
				// Activa el relé con lógica inversa 
				case 0:
					Serial.println("Case 0");
					//LED en NODEMCU con lógica inversa
					digitalWrite(LED_BUILTIN, router);
					digitalWrite(4, LOW);
					break;
				// Desactiva el relé con lógica inversa
				case 1:
					Serial.println("Case 1");
					//LED en NODEMCU con lógica inversa
					digitalWrite(LED_BUILTIN, router);
					digitalWrite(4, HIGH);
					break;
				// Agrega número en SIM
				case 2:
					Serial.println("Case 2");
					
					// Limpia variable temporal
					tmpx = "";

					// Limpia indexAndName
					indexAndName = "";
					
					// Limpia newContact
					newContact = "";

					// Asigna posición del sim que se va a incorporar en el sistema
					indexAndName = add_del;

					// Asigna número telefónico que se va a incorporar en el sistema
					newContact = tel;

					// Guarda número en SIM
					tmpx = "AT+CPBW=" + indexAndName + ",\"" + newContact + "\"" + ",129," + "\"" + indexAndName + "\"" + "\r\n\"";
					tmpx.toCharArray( aux_string, 100 );
					answer = sendATcommand(aux_string, "OK", 20000, 0);
					if (answer == 1)
					{
						Serial.println("Agregado en el sistema ");
					}
					else
					{
						Serial.println("Error, verifique formato del string ");
					}
					break;
				// Borra número en SIM
				case 3:
					Serial.println("Case 3");

					// Limpia variable temporal
					tmpx = "";

					// Limpia indexAndName
					indexAndName = "";

					// Asigna posición del sim que se va a borrar
					indexAndName = add_del;
					
					tmpx = "AT+CPBW=" + indexAndName + "\r\n\"";
					tmpx.toCharArray( aux_string, 100 );
					answer = sendATcommand(aux_string, "OK", 20000, 0);
					if (answer == 1)
					{
						Serial.println("Borrado del sistema");
					}
					else
					{
						Serial.println("Error, verifique formato del string ");
					}
					break;
				default:
				break;
			}
			sendSMS  (telx,msgx) ;
		}
    }
  } 
  else 
  {
	Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
  }
	http.end();
  }   
}