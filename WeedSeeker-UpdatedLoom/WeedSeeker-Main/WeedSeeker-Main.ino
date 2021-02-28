///////////////////////////////////////////////////////////////////////////////

// FileName: WeedSeeker-Main.ino 
// FileType: Arduino Source code file
// Author: Liam Duncan
// Organization: Oregon State Open Sensing Lab (OPEnS) 
// Last Modified: 2/28/2021 

// Description: 
// This is the code for the OPEnS Lab Weed Warden. 
// This code is meant to be uploaded to the feather M0 board and used to detect vegetation. 
// Default setting for this code are to use the NDVIB algorithm and a threshold of .015 over the calibration value

///////////////////////////////////////////////////////////////////////////////

// Include the OPEnS Lab Loom library 
// https://github.com/OPEnSLab-OSU/Loom/wiki/Quick-Start
#include <Loom.h>


// Include configuration
const char* json_config = 
#include "config.h"
;

#define VBATPIN A7           // pin connected to the 3.7V battery 
#define 3.3V_Rail_Enable 5   // pin 5 is the 3.3V Enable
#define 12V_Rail_Enable 6    // pin 6 is the 5V and V+ Enable  

// Set enabled modules
LoomFactory<
	Enable::Internet::Disabled,
	Enable::Sensors::Enabled,
	Enable::Radios::Disabled,
	Enable::Actuators::Disabled,
	Enable::Max::Disabled
> ModuleFactory{};

LoomManager Loom{ &ModuleFactory };

// Make global variables for calibration variables
 float threshold = 0;   // variable to hold the calibration threshold
 float ndvi[16];        // array to hold the NDVI values during calibration
 float offset = .015;   // This value is the threshold offset 


// Setup Function
void setup() 
{ 
  
  pinMode(3.3V_Rail_Enable, OUTPUT);    // 3.3V_Rail_Enable is the pin that enables the hypnos board 3.3V 
  pinMode(12V_Rail_Enable, OUTPUT);     // 12V_Rail_Enable is the variable that will control the 5V and 12V Rails on the Hypnos board 

  // 3.3V Enable is ACTIVE LOW.
  digitalWrite(3.3V_Rail_Enable, LOW);  // Turn on the 3.3V Rail for I2C control and SD card logging

  // 5V/12V rail is ACTIVE HIGH
  digitalWrite(12V_Rail_Enable, LOW);   // Start with the 12V_Rail_Enable off 

  // Configure Loom and Serial 
	Loom.begin_serial(true);
  Serial.begin(9600); 
	Loom.parse_config(json_config);
	Loom.print_config();

	LPrintln("\n ** Setup Complete ** ");
   
} // Setup 



//================================================================
//===                     calibrate                           ====
//===                                                         ====
//=== Function will take samples of dirt to find a baseline   ====
//=== NDVI Value that can be used as a threshold              ==== 
//================================================================
void calibrate()
{  
  for(int z=0; z<16; z++) // Loop to take calibration samples
  {
    Loom.measure();   // Take data from the spectral triad sensor   

    JsonObject calibrationData = Loom.package();    // Store data in JSON package

    serializeJsonPretty(calibrationData, Serial);   // Serialize data 

    // The following lines take the spectral data from the array and store them in 
    // individual values. The wavelengths are listed on the side
    JsonObject as7265x = (calibrationData["contents"].as<JsonArray>())[1]["data"];
    uint16_t a = as7265x["a"]; a = (float)a; // 410
    uint16_t b = as7265x["b"]; b = (float)b; // 435
    uint16_t c = as7265x["c"]; c = (float)c; // 460
    uint16_t d = as7265x["d"]; d = (float)d; // 485
    uint16_t e = as7265x["e"]; e = (float)e; // 510
    uint16_t f = as7265x["f"]; f = (float)f; // 535
    uint16_t g = as7265x["g"]; g = (float)g; // 560
    uint16_t h = as7265x["h"]; h = (float)h; // 585
    uint16_t i = as7265x["i"]; i = (float)i; // 645
    uint16_t j = as7265x["j"]; j = (float)j; // 705
    uint16_t k = as7265x["k"]; k = (float)k; // 900
    uint16_t l = as7265x["l"]; l = (float)l; // 940
    uint16_t r = as7265x["r"]; r = (float)r; // 610
    uint16_t s = as7265x["s"]; s = (float)s; // 680
    uint16_t t = as7265x["t"]; t = (float)t; // 730
    uint16_t u = as7265x["u"]; u = (float)u; // 760
    uint16_t v = as7265x["v"]; v = (float)v; // 810
    uint16_t w = as7265x["w"]; w = (float)w; // 860


   // calculate ndvi value 
   // This is where the user would change the algorithm for calibration 

   float nd = ((u + v + w + e + f + g) - 2*(b + c + d));  // calculate top half of NDVI 
   float vib = ((u + v + w + e + f + g) + 2*(b + c + d)); // Calculate bottom half of NDVI
   ndvi[z] = nd/vib;  // Store NDVI values in an array

   LPrintln(ndvi[z]);   // print out the calculated NDVI value
   
  } // for loop 

  // loop to sum up the ndvi values 
  for(int i=0; i<16; i++)
    threshold += ndvi[i];
  
  threshold = (threshold / 16) + offset;  // divide by 16 to average values then add an offset to the value

  // Turn on the 12V_Rail_Enable for 6 seconds so that you know the calibration worked
  // In our tests we had the 5V rail hooked up to a 5V buzzer so that we knew when the calibration had finished
  digitalWrite(12V_Rail_Enable, HIGH); 
  delay(6000);               
  digitalWrite(12V_Rail_Enable, LOW); 
} // Calibrate 



//================================================================
//===                     Measure battery                     ====
//===                                                         ====
//=== Function returns the battery voltage of a 3.7V battery ====
//=== that is connected to the JST connector of the Feather   ==== 
//================================================================
int MeasureBat()
{
  // Read battery pin
  float measuredvbat = analogRead(VBATPIN);  

  // Perform some calculations as specified by the Feather M0 guide 
  measuredvbat *= 2; 
  measuredvbat *= 3.3; 
  measuredvbat /= 1024; 

  // Return Battery voltage
  return measuredvbat; 
}


// Main loop! 
void loop()
{

  // Run the calibration function
  calibrate(); 

  while(1){

    // Measure the data from sensor 
    Loom.measure();                         

    // Store data in JSON package
    JsonObject SensorData = Loom.package(); 

    // Add the state of the Growlight to the file 
    Loom.add_data("LightState", "LightState", LightState);

    // Serialize the JSON Object and print it to the Serial Monitor
    serializeJsonPretty(SensorData, Serial); 

    // put the spectral values into their own variables
    Serial.println("+++++ START +++++");
    JsonObject as7265x = (SensorData["contents"].as<JsonArray>())[1]["data"];
    uint16_t a = as7265x["a"]; a = (float)a; //Serial.println(a); // 410
    uint16_t b = as7265x["b"]; b = (float)b; //Serial.println(b); // 435
    uint16_t c = as7265x["c"]; c = (float)c; //Serial.println(c); // 460
    uint16_t d = as7265x["d"]; d = (float)d; //Serial.println(d); // 485
    uint16_t e = as7265x["e"]; e = (float)e; //Serial.println(e); // 510
    uint16_t f = as7265x["f"]; f = (float)f; //Serial.println(f); // 535
    uint16_t g = as7265x["g"]; g = (float)g; //Serial.println(g); // 560
    uint16_t h = as7265x["h"]; h = (float)h; //Serial.println(h); // 585
    uint16_t i = as7265x["i"]; i = (float)i; //Serial.println(i); // 645
    uint16_t j = as7265x["j"]; j = (float)j; //Serial.println(j); // 705
    uint16_t k = as7265x["k"]; k = (float)k; //Serial.println(k); // 900
    uint16_t l = as7265x["l"]; l = (float)l; //Serial.println(l); // 940
    uint16_t r = as7265x["r"]; r = (float)r; //Serial.println(r); // 610
    uint16_t s = as7265x["s"]; s = (float)s; //Serial.println(s); // 680
    uint16_t t = as7265x["t"]; t = (float)t; //Serial.println(t); // 730
    uint16_t u = as7265x["u"]; u = (float)u; //Serial.println(u); // 760
    uint16_t v = as7265x["v"]; v = (float)v; //Serial.println(v); // 810
    uint16_t w = as7265x["w"]; w = (float)w; //Serial.println(w); // 860

    // Sum up the total of all wavelengths
    float total = a + b + c + d + e + f + g + h + i + j + k + l + r + s + t + u + v + w;
    
    // calculate normalized wavelength values
    Serial.println("++++++++++++++++");
    Serial.println("++++++++++++++++");    
    float a_w = a/total; //Serial.println(a_w);
    float b_w = b/total; //Serial.println(b_w);
    float c_w = c/total; //Serial.println(c_w);
    float d_w = d/total; //Serial.println(d_w);
    float e_w = b/total; //Serial.println(e_w);
    float f_w = b/total; //Serial.println(f_w);
    float g_w = g/total; //Serial.println(g_w);
    float j_w = j/total; //Serial.println(j_w); 
    float k_w = k/total; //Serial.println(k_w);
    float r_w = r/total; //Serial.println(r_w);
    float s_w = s/total; //Serial.println(s_w);
    float t_w = t/total; //Serial.println(t_w);
    float u_w = u/total; //Serial.println(u_w);
    float v_w = v/total; //Serial.println(v_w);
    float w_w = w/total; //Serial.println(w_w);

    // calculate the NDVIB Value
    // This is where the user would need to change the code 
    // in order to use a different detection algorithm
    float nd = ((u + v + w + e + f + g) - 2*(b + c + d));
    float vib = ((u + v + w + e + f + g) + 2*(b + c + d));
    float ndvib = nd/vib; 

    // Calculate some other indices that are not used

    // PSND
    float ps = v-f;
    float nd2 = v+f;
    float psnd = ps/nd2;

    // EVI   
    float evi = ((2.5*(v-s))/(v+6*s-7.5*b+1));

    // Add Values to the JSON Package so that they will be logged to SD Card
    Loom.add_data("NDVIB", "NDVIB", ndvib);
    Loom.add_data("EVI", "EVI", evi);
    Loom.add_data("Threshold", "Threshold", threshold);

    // Calculate normalized NDVIB 
    float ndvib_w = ((u_w + v_w + w_w + e_w + f_w + g_w) - 2*(b_w + c_w + d_w))/((u_w + v_w + w_w + e_w + f_w + g_w) + 2*(b_w + c_w + d_w));
    
    // Calculate normalized psnd 
    float psnd_w = ((v_w-f_w)/(v_w+f_w));

    // Calculate normalized EVI 
    float evi_w = ((2.5*(v_w-s_w))/(v_w+6*s_w-7.5*b_w+1));

    // Add Values to the JSON Package
    Loom.add_data("NDVIB_W", "NDVIB_W", ndvib_w);
    Loom.add_data("EVI_W", "EVI_W", evi_w);
    Loom.add_data("PSND_W", "PSND_W", psnd_w);

    // Log the data to SD Card 
    Loom.SDCARD().log(); 

    // Print to the serial monitor
    Serial.println("++++++++++++++++");
    LPrint("Total = "); LPrintln(total);
    
    LPrint("ndvi = "); LPrintln(ndvib);
    LPrint("threshold = "); LPrintln(threshold);  


    // Compare the threshold value to the measured value to determine the presence of vegetation
    if(threshold < ndvib)
    {

        Serial.println("++++++++++++++++++++++++++++++++++++++");
        Serial.println("++++++++++ This Is A Target ++++++++++");
        Serial.println("++++++++++++++++++++++++++++++++++++++");

        digitalWrite(12V_Rail_Enable, HIGH);   // Turn on the 12V_Rail_Enable if grass is detected

    } // if
    else {
      Serial.println("----------------------------------");
      Serial.println("++++++++++ Not A Target ++++++++++");
      Serial.println("----------------------------------");

      digitalWrite(12V_Rail_Enable, LOW);  // Turn the 12V_Rail_Enable off if dirt is detected
    } // else

    Serial.println("+++++ End +++++"); 
    Loom.pause();
  
  } // while 

} // main loop 
