#include <ros/ros.h>
#include <sensor_msgs/Joy.h>
#include <wiringPi.h>
#include <iostream>
#include <std_msgs/UInt8MultiArray.h> 
#include <math.h>
#include <serial/serial.h>
#include <stdint.h>

using namespace std;

//define toggle switches and LED light pins on raspberry pi 3B+ GPIO
#define TOGGLE_DRIVE_MAN    0 	// Toggle switch to switch between manipulation and drive control
#define TOGGLE_AUTO_TEL     2   // Toggle switch to switch between autonomous and teleoperation driving modes
#define MAN_LED      3  // LED light to signal Manipulation mode
#define TEL_LED      4  // LED light to signal Teleoperation
#define AUTO_LED     5  // LED light to signal Autonomous mode

#define SERIAL_PORT_NAME     "/dev/uart2ppm" // something like "/dev/ttyACM0" or "/dev/" (device is "cp210x converter")
#define N         13  // The number of bytes (or the number of elements) inside the serial_array that is being serialized

std_msgs::UInt8MultiArray ss_out;
std_msgs::MultiArrayDimension ss_out_specs;
   

/* Subsc contains two functions that subscribe to the ROS topics "joy_nav_values" and "man_values". The values from the ROS topic messages are saved to the serial data . */
class Subsc
{ public:
  
  uint8_t Nav_msg[6] = {127,127,127,127,127,127};   //Stored array values from navigation's joystick values.
  uint8_t Man_msg[6] = {100,100,100,95,100,100};  //Stored array values from manipulations joystick values
};



/*void Subsc::drive_Button(const sensor_msgs::Joy::ConstPtr& joy){
 
 this -> button_x_tel = joy -> buttons[X_BUTTON];
} */

Subsc s; // Global variable for arrays
serial::Serial ser;

void NavMsg(const std_msgs::UInt8MultiArray::ConstPtr& msg_1){
 for (int ii=0;ii<6;ii++){
   s.Nav_msg[ii] = msg_1->data[ii];      // [3]
 }
 // cout << (s_1.Nav_msg) << endl;
}

void ManMsg(const std_msgs::UInt8MultiArray::ConstPtr& msg_2){
 for (int jj=0;jj<6;jj++){
   s.Man_msg[jj] = msg_2->data[jj];
 }
}

//Function to initialize the serial port
bool InitSerial(uint32_t baud, std::string portName) {
    serial::Timeout timeout = serial::Timeout(0, 0, 0, 0, 0);
    serial::bytesize_t bytesize = serial::eightbits;
    serial::parity_t parity = serial::parity_none;
    serial::stopbits_t stopbits = serial::stopbits_one;
    serial::flowcontrol_t flowcontrol = serial::flowcontrol_none;

    try{
        ser.setPort(portName);
        ser.setBaudrate(baud);
        ser.setTimeout(timeout);
        ser.setBytesize(bytesize);
        ser.setParity(parity);
        ser.setStopbits(stopbits);
        ser.setFlowcontrol(flowcontrol);
        ser.open();
    }
    catch (serial::SerialException e) {
        ROS_FATAL("Failed to connect to the Motor Controller, %s.", e.what());
        ros::shutdown();	//Halt the ROS system when critical components are not functioning
        return false;
    }
    return true;
}

void serial_fun(const uint8_t *message,int num_msg_elem,uint8_t drive, uint8_t auto_teleops) {
  ros::NodeHandle nn;
 uint8_t serial_array[13];
 uint8_t pid_1 = 'G';
 uint8_t pid_2 = 'O';
 uint8_t pid_3 = 0;
 uint8_t Data_Array_Size = 8;  // Number of array members that makes up the data being transmitted. 

 serial_array[0] = pid_1;
 serial_array[1] = pid_2;
 serial_array[2] = pid_3;
 serial_array[3] = Data_Array_Size;
 serial_array[4] = drive;
 serial_array[5] = auto_teleops;



 for (uint8_t i = 6; i < 12; i++) {
    serial_array[i] = *message;  //[4]
    message++;
  }


 uint8_t crc = 0;

 for (int k=3;k<12;k++){
   for (int m=0;m<8;m++){
     crc += (serial_array[k]>>m)&0x01;  //Checks all the data values in the serial_array for 1 bits using "shift"
   }
  }
 
 serial_array[12] = crc;
 cout << "Serial contents\n";
 for (int i = 0; i < 13; i++) {
   cout << unsigned(serial_array[i]) << " ";
 }
 cout << endl;

 ser.write(serial_array, N); //Send Packet over serial

 /*
  ros::Publisher joy_publish = nn.advertise<std_msgs::UInt8MultiArray>("serial_out", 1);

  ros::Rate rate(20);
  while (ros::ok()) {
      ss_out_specs.label = "serial_val";
      ss_out_specs.size = 13;
      ss_out.layout.dim.clear();
      ss_out.layout.dim.push_back(ss_out_specs);

      ss_out.data.clear();

/* "push_back" only puts one value at a time into the data array in "UInt8MultiArray. The first term entered will be the last term in the array.[3] 
      for (int h=0;h<13;h++){
        ss_out.data.push_back(serial_array[h]);
      }

      joy_publish.publish(ss_out);
   
      ros::spinOnce();

      rate.sleep();
   }
*/

 }
 


int main (int argc, char **argv){

 // Call in Toggle value from GPIO pin

 wiringPiSetup();        // Ref [1]
 pinMode(TOGGLE_DRIVE_MAN, INPUT);
 pinMode(TOGGLE_AUTO_TEL, INPUT);
 pinMode(MAN_LED, OUTPUT);
 pinMode(AUTO_LED, OUTPUT);
 pinMode(TEL_LED, OUTPUT);


 uint8_t Drive_Man ;
 uint8_t Auto_Tel ;
 int num_msg_elem = 6;

 ros::init(argc, argv, "joy_serial");
 ros::NodeHandle n;

  ros::Subscriber Nav = n.subscribe("nav_joy_values",1,NavMsg);
 ros::Subscriber Man = n.subscribe("manipulation_data",1,ManMsg);

InitSerial(9600, SERIAL_PORT_NAME); //initialize serial port with 9600 baud rate

 ros::Rate rate(20);
        
         while (ros::ok()) {
            Drive_Man = digitalRead(TOGGLE_DRIVE_MAN);   // Ref [2]
            Auto_Tel = digitalRead(TOGGLE_AUTO_TEL);
            // Drive_Man = 1;
            // Auto_Tel = 1;
             if (Drive_Man == 1){
                  Drive_Man = 255;
             }
             if (Auto_Tel == 1){
                  Auto_Tel = 255;
             }

           if (Auto_Tel==255){
	      digitalWrite(AUTO_LED,HIGH);
	      digitalWrite(TEL_LED,LOW);
            }
  
          else {
	    digitalWrite(AUTO_LED,LOW);
	    digitalWrite(TEL_LED,HIGH);
           }

          if (Drive_Man == 0){
	    digitalWrite(MAN_LED,HIGH);
	    uint8_t man[6] = {127,127,127,127,127,127};
	     serial_fun(s.Man_msg, num_msg_elem, Drive_Man, Auto_Tel);
           }
    
           else if (Drive_Man == 255){
             digitalWrite(MAN_LED,LOW);
              if ( Auto_Tel == 0) {
                   
                   serial_fun(s.Nav_msg, num_msg_elem, Drive_Man, Auto_Tel);
               }

             else if (Auto_Tel == 255) {
                   uint8_t auto_msg[6] = {127,127,127,127,127,127};
                   serial_fun(auto_msg, num_msg_elem, Drive_Man, Auto_Tel);
              }
            }

           ros::spinOnce();
           rate.sleep();
   
        }
  digitalWrite(MAN_LED,LOW);
  digitalWrite(AUTO_LED,LOW);
  digitalWrite(TEL_LED,LOW);
 ser.close();
 return 0;
}


/* References 
[1] https://www.digikey.com/en/maker/blogs/2019/how-to-use-gpio-on-the-raspberry-pi-with-c
[2] https://github.com/WiringPi/WiringPi/blob/master/wiringPi/wiringPi.h
[3] https://raw.githubusercontent.com/durovsky/siemens_tutorials/master/siemens_cp1616_io_device_tutorial/doc/doc_siemens_cp1616_io_device_tutorial_node.cpp
[4] Passing arrays in with pointers [https://beginnersbook.com/2014/01/c-passing-array-to-function-example/]
*/
