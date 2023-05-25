import SimpleOpenNI.*;
import processing.serial.*;

Serial port;
private static final int SERIAL_PORT_INDEX = 2;
private static final int BAUD_RATE = 9600;

SimpleOpenNI kinect;

private static final int playerID = 1;

private static final byte BYTE_START = 'S';
private static final byte BYTE_NOT_DETECTED = 0;
private static final byte BYTE_RECOGNISED = 'R';
private static final byte BYTE_START_PLAYING = 'G';
private static final byte BYTE_LOST = 'L';

int previousMillis = 0;
private static final int interval = 4000;

private static final int SCREEN_WIDTH = 640;
private static final int SCREEN_HEIGHT = 480;

private static final int STATUS_NO_PLAYER = 0;
private static final int STATUS_PLAYER_RECOGNISED = 1;
private static final int STATUS_PLAYING = 2;
int status = STATUS_NO_PLAYER;

boolean scanningPlayer;
boolean losingPlayer;

void setup() {
  kinect = new SimpleOpenNI(this);
  kinect.enableDepth();
  kinect.enableUser();

  kinect.enableHand();
  kinect.startGesture(SimpleOpenNI.GESTURE_WAVE);
  
  size(640, 480);
  fill(255, 0, 0);
 
  String portName = Serial.list()[SERIAL_PORT_INDEX];
  port = new Serial(this, portName, BAUD_RATE);
}

void draw() {
  kinect.update();
  image(kinect.depthImage(), 0, 0);
  
  if (status != STATUS_PLAYING) {
    return;
  }
  updateSkeleton();
}

PVector getProjectiveJoint(int userID, int jointID) {
  PVector joint = new PVector();
  float confidence = kinect.getJointPositionSkeleton(userID, jointID, joint);
  
  PVector convertedJoint = new PVector();
  kinect.convertRealWorldToProjective(joint, convertedJoint);
  
  ellipse(convertedJoint.x, convertedJoint.y, 20, 20);
  return convertedJoint;
}

float between(float val, float lowerBound, float upperBound) {
  val = max(lowerBound, val);
  val = min(val, upperBound);
  return val;
}

void sendCoordinateAxis(float value, int lowerBound, int upperBound) {
  int intVal = floor(between(value, lowerBound, upperBound));
  int asByte = (int)map(intVal, lowerBound, upperBound, 0, 255);
  port.write(asByte);
}

void send2DJoint(int userID, int jointID) {
  PVector projectiveJoint = getProjectiveJoint(userID, jointID);
  
  if (projectiveJoint == null) {
    port.write(BYTE_NOT_DETECTED);
    port.write(BYTE_NOT_DETECTED);
    return;
  }
  
  sendCoordinateAxis(projectiveJoint.x, SCREEN_WIDTH / 4, SCREEN_WIDTH / 4 * 3);
  sendCoordinateAxis(projectiveJoint.y, 0, SCREEN_HEIGHT);
}

void updateSkeleton() {
  if (losingPlayer) {
    return;
  }
  scanningPlayer = true;
  
  int currentMillis = millis();
  if (!kinect.isTrackingSkeleton(playerID) || currentMillis - previousMillis <= interval) {
    return;
  }
  previousMillis = currentMillis;
  
  port.write(BYTE_START);
  
  send2DJoint(playerID, SimpleOpenNI.SKEL_HEAD);
  send2DJoint(playerID, SimpleOpenNI.SKEL_NECK);
  
  send2DJoint(playerID, SimpleOpenNI.SKEL_LEFT_SHOULDER);
  send2DJoint(playerID, SimpleOpenNI.SKEL_LEFT_ELBOW);
  send2DJoint(playerID, SimpleOpenNI.SKEL_LEFT_HAND);
  
  send2DJoint(playerID, SimpleOpenNI.SKEL_RIGHT_SHOULDER);
  send2DJoint(playerID, SimpleOpenNI.SKEL_RIGHT_ELBOW);
  send2DJoint(playerID, SimpleOpenNI.SKEL_RIGHT_HAND);
  
  send2DJoint(playerID, SimpleOpenNI.SKEL_TORSO);
  
  send2DJoint(playerID, SimpleOpenNI.SKEL_LEFT_HIP);
  send2DJoint(playerID, SimpleOpenNI.SKEL_LEFT_KNEE);
  send2DJoint(playerID, SimpleOpenNI.SKEL_LEFT_FOOT);
  
  send2DJoint(playerID, SimpleOpenNI.SKEL_RIGHT_HIP);
  send2DJoint(playerID, SimpleOpenNI.SKEL_RIGHT_KNEE);
  send2DJoint(playerID, SimpleOpenNI.SKEL_RIGHT_FOOT);
  
  scanningPlayer = false;
}

void onCompletedGesture(SimpleOpenNI kinect, int gestureType, PVector pos) {
  if (gestureType == SimpleOpenNI.GESTURE_WAVE && status == STATUS_PLAYER_RECOGNISED) {
    println("Player waved. Starting.");
    port.write(BYTE_START_PLAYING);
    status = STATUS_PLAYING;
  }
}

void onNewUser(SimpleOpenNI kinect, int userID){
  println("onNewUser - userID: " + userID);
  if (kinect.isTrackingSkeleton(playerID) || userID != playerID) {
    return;
  }
  
  println("Detected player, wave to start!");
  status = STATUS_PLAYER_RECOGNISED;
  port.write(BYTE_RECOGNISED);
  kinect.startTrackingSkeleton(userID);
}

void onLostUser(SimpleOpenNI kinect, int userID) {
  println("onLostUser - userID: " + userID);
  if (userID != playerID) {
    return;
  }
  
  println("Player lost");
  status = STATUS_NO_PLAYER;
  
  losingPlayer = true;
  kinect.stopTrackingSkeleton(userID);

  for (int i = 0; i < 31; i++) {
    port.write(BYTE_LOST);
  } 
  losingPlayer = false;
}