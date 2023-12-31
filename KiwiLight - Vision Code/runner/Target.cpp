#include "Runner.h"

/**
 * Source file for the Target class.
 * Written By: Brach Knutson
 */

using namespace cv;
using namespace KiwiLight;

/**
 * Creates a new Target, initalized with "null" values.
 * NOTE: DO NOT use this constructor to find or define a target.
 * Targets will be generated by the ExampleTarget class.Use an
 * ExampleTarget object to find targets!!!
 */
Target::Target() {
    this->id = -1;
    this->contours = std::vector<Contour>();
    this->knownHeight = -1;
    this->focalHeight = -1;
    this->distErrorCorrect = -1;
    this->calibratedDistance = -1;
    this->width = -1;
    this->height = -1;
    this->x = -1;
    this->y = -1;
}

/**
 * Creates a new Target object with the vector of contours. 
 * NOTE: DO NOT use this constructor to find or define a target.
 * Targets will be generated by the ExampleTarget class. Use an
 * ExampleTarget object to find targets!!!
 */
Target::Target(int id, std::vector<Contour> contours, double knownHeight, double focalHeight, double distErrorCorrect, double calibratedDistance, DistanceCalcMode distMode) {
    this->id = id;
    this->contours = contours;
    this->knownHeight = knownHeight;
    this->focalHeight = focalHeight;
    this->distErrorCorrect = distErrorCorrect;
    this->calibratedDistance = calibratedDistance;
    this->distMode = distMode;

    if(contours.size() == 1) {
        this->width = contours[0].Width();
        this->height = contours[0].Height();
        this->x = (this->width / 2) + contours[0].X();
        this->y = (this->height / 2) + contours[0].Y();
    } else {
        int biggestX = -5000;
        int smallestX = 5000;
        int biggestY = -5000;
        int smallestY = 5000;

        int biggestXWidth = 0;
        int biggestYHeight = 0; 

        for(int i=0; i<contours.size(); i++) {
            if(contours[i].X() > biggestX) {
                biggestX = contours[i].X();
                biggestXWidth = contours[i].Width();
            } 
             if(contours[i].X() < smallestX) {
                smallestX = contours[i].X();
            }

            if(contours[i].Y() > biggestY) {
                biggestY = contours[i].Y();
                biggestYHeight = contours[i].Height();
            }
             if(contours[i].Y() < smallestY) {
                smallestY = contours[i].Y();
            }
        }

        this->width = (biggestX - smallestX) + biggestXWidth;
        this->height = (biggestY - smallestY) + biggestYHeight;
        this->x = (this->width / 2) + smallestX;
        this->y = (this->height / 2) + smallestY;
    }
}

/**
 * Returns the distance from the camera to the target (in whatever units distance was calibrated in). Uses width for calculations by default.
 */
double Target::Distance() { 
    return Distance(distMode);
}

/**
 * Returns the distance from the camera to the target (in whatever units distance was calibrated in). Uses specified mode (width or height)
 * @param mode The DistanceCalcMode to calculate distance by. BY_WIDTH will use the width to calculate distance, BY_HEIGHT will use height.
 */
double Target::Distance(DistanceCalcMode mode) {
    //calculate distance (formula: known(in) * focal(in) / real(px))
    double referenceLength = (double) (mode == DistanceCalcMode::BY_WIDTH ? this->Bounds().width : this->Bounds().height);
    double dist = this->knownHeight * this->focalHeight / referenceLength;

    double err = this->calibratedDistance - dist;
    err *= this->distErrorCorrect;
    
    return (dist + err);
}

/**
 * Returns the angle (in degrees) the robot needs to turn horizontally to be considered "aligned" with the target.
 * @param distanceToTarget The Distance to the target.
 * @param imageCenterX The x coordinate of the center of the image. (width / 2)
 */
double Target::HorizontalAngle(double distanceToTarget, int imageCenterX) {
    double inchesPerPixel = this->knownHeight / this->Bounds().width;
    int pixelsToTarget = imageCenterX - this->Center().x;

    double inchesToTarget = pixelsToTarget * inchesPerPixel;
    double angle = atan(inchesToTarget / distanceToTarget);

    //convert to degrees
    angle *= (180 / M_PI);
    return angle;
}

/**
 * Returns the angle (in degrees) the robot needs to turn horizontally to be considered "aligned" with the target.
 * @param imageCenterX The x coordinate of the center of the image (width / 2)
 */
double Target::HorizontalAngle(int imageCenterX) {
    return this->HorizontalAngle(this->Distance(), imageCenterX);
}

/**
 * Returns the angle (in degrees) the robot needs to turn vertically to be considered "aligned" with the target.
 * @param distancEtoTarget the distance to the target.
 * @param imageCenterY the Y coordinate of the center of the image.
 */
double Target::VerticalAngle(double distanceToTarget, int imageCenterY) {
    double InchesPerPixel = this->knownHeight / this->Bounds().width;
    int pixelsToTarget = imageCenterY - this->Center().y;
    
    double inchesToTarget = pixelsToTarget * InchesPerPixel;
    double angle = atan(inchesToTarget / distanceToTarget);

    //convert to degrees and return
    angle *= (180 / M_PI);
    return angle;
}

/**
 * Returns the angle (in degrees) the robot needs to turn vertically to be considered "aligned" with the target.
 * @param imageCenterY The Y coordinate of the center of the image.
 */
double Target::VerticalAngle(int imageCenterY) {
    return this->VerticalAngle(this->Distance(), imageCenterY);
}

/**
 * Returns the angle in 3-D space that the robot needs to turn to be considered "aligned" with the target.
 * @param imageCenterX the X coordinate of the center of the image.
 * @param imageCenterY the Y coordinate of the center of the image.
 */
double Target::ObliqueAngle(int imageCenterX, int imageCenterY) {
    double horizontalAngle = this->HorizontalAngle(imageCenterX) * (M_PI / 180); //convert angle from degrees to radians
    double verticalAngle   = this->VerticalAngle(imageCenterY)   * (M_PI / 180);
    double targetDistance  = this->Distance();

    double horizontalOffset = targetDistance * tan(horizontalAngle);
    double verticalOffset   = targetDistance * tan(verticalAngle);
    double obliqueOffset = sqrt (
        pow(horizontalOffset, 2) +
        pow(verticalOffset, 2)
    );

    double obliqueAngle = atan(obliqueOffset / targetDistance);

    //convert the angle back to degrees
    obliqueAngle *= (180 / M_PI);
    return obliqueAngle;
}

/**
 * Returns a rectangle that represents the bounds of the target.
 */
cv::Rect Target::Bounds() {
    //find the corner x and y because the local x and y are for the center
    int trueX = this->x - (this->width / 2);
    int trueY = this->y - (this->height / 2);

    return cv::Rect(trueX, trueY, this->width, this->height);
}
