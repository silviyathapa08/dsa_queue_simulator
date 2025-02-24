#ifndef TRAFFIC_SIMULATION_H
#define TRAFFIC_SIMULATION_H

#include <SDL.h>
#include <stdbool.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600
#define LANE_WIDTH 80
#define MAX_VEHICLES 10
#define INTERSECTION_X (WINDOW_WIDTH / 2)
#define INTERSECTION_Y (WINDOW_HEIGHT / 2)

typedef enum {
    DIRECTION_NORTH,
    DIRECTION_SOUTH,
    DIRECTION_EAST,
    DIRECTION_WEST
} Direction;
typedef enum {
    TURN_NONE,
    TURN_LEFT,
    TURN_RIGHT
} TurnDirection;

typedef enum {
    STATE_MOVING,
    STATE_STOPPING,
    STATE_STOPPED,
    STATE_TURNING
} VehicleState;
typedef enum {
    REGULAR_CAR,
    AMBULANCE,
    POLICE_CAR,
    FIRE_TRUCK
} VehicleType;

typedef enum {
    RED,
    GREEN
} TrafficLightState;

#define TRAFFIC_LIGHT_WIDTH (LANE_WIDTH * 2)
#define TRAFFIC_LIGHT_HEIGHT (LANE_WIDTH - LANE_WIDTH / 3)
#define STOP_LINE_WIDTH 5

typedef struct {
    SDL_Rect rect;
    VehicleType type;
    Direction direction;
    TurnDirection turnDirection;
    VehicleState state;
    float speed;
    float x;
    float y;
    bool active;
    float turnAngle;  
    bool isInRightLane;
    bool turnProgress;
} Vehicle;
