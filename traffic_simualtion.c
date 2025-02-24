#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "traffic_simulation.h"

// Global queues for lanes
Queue laneQueues[4];         // Queues for lanes A, B, C, D
int lanePriorities[4] = {0}; // Priority levels for lanes (0 = normal, 1 = high)

const SDL_Color VEHICLE_COLORS[] = {
    {0, 0, 255, 255}, // REGULAR_CAR: Blue
    {255, 0, 0, 255}, // AMBULANCE: Red
    {0, 0, 128, 255}, // POLICE_CAR: Dark Blue
    {255, 69, 0, 255} // FIRE_TRUCK: Orange-Red
};
//vehicle addition
// renderVehicle(renderer, vehicleType, vehicleRect.x, vehicleRect.y, vehicleRect.w, vehicleRect.h);
//tried keeping imaeges but was too frustating so goig with the rectangular blocks!! 


void initializeTrafficLights(TrafficLight *lights)
{
    lights[0] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_NORTH};
    lights[1] = (TrafficLight){
        .state = RED,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH, TRAFFIC_LIGHT_WIDTH, TRAFFIC_LIGHT_HEIGHT},
        .direction = DIRECTION_SOUTH};
    lights[2] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_EAST};
        //traffic light generation 
    lights[3] = (TrafficLight){
        .state = GREEN,
        .timer = 0,
        .position = {INTERSECTION_X - LANE_WIDTH - TRAFFIC_LIGHT_HEIGHT, INTERSECTION_Y - LANE_WIDTH, TRAFFIC_LIGHT_HEIGHT, TRAFFIC_LIGHT_WIDTH},
        .direction = DIRECTION_WEST};
}
void updateTrafficLights(TrafficLight *lights)
{
    Uint32 currentTicks = SDL_GetTicks();
    static Uint32 lastUpdateTicks = 0;

    if (currentTicks - lastUpdateTicks >= 5000)
    { // Change lights every 5 seconds
        lastUpdateTicks = currentTicks;

        // Check for high-priority lanes
        for (int i = 0; i < 4; i++)
        {
            if (laneQueues[i].size > 10)
            {
                lanePriorities[i] = 1; // Set high priority
            }
            else if (laneQueues[i].size < 5)
            {
                lanePriorities[i] = 0; // Reset to normal priority
            }
        }

        // Toggle lights based on priority
        for (int i = 0; i < 4; i++)
        {
            if (lanePriorities[i] == 1)
            {
                lights[i].state = GREEN; // Give green light to high-priority lane
            }
            else
            {
                lights[i].state = (lights[i].state == RED) ? GREEN : RED; // Toggle lights
            }
        }
    }
}

Vehicle *createVehicle(Direction direction)
{
    Vehicle *vehicle = (Vehicle *)malloc(sizeof(Vehicle));
    vehicle->direction = direction;

    // Set vehicle type with probabilities
    int typeRoll = rand() % 100;
    if (typeRoll < 5)
    {
        vehicle->type = AMBULANCE;
    }
    else if (typeRoll < 10)
    {
        vehicle->type = POLICE_CAR;
    }
    else if (typeRoll < 15)
    {
        vehicle->type = FIRE_TRUCK;
    }
    else
    {
        vehicle->type = REGULAR_CAR;
    }
    vehicle->active = true;
    // Set speed based on vehicle type
    switch (vehicle->type)
    {
    case AMBULANCE:
    case POLICE_CAR:
        vehicle->speed = 4.0f;
        break;
    case FIRE_TRUCK:
        vehicle->speed = 3.5f;
        break;
    default:
        vehicle->speed = 2.0f;
    }

    vehicle->state = STATE_MOVING;
    vehicle->turnAngle = 0.0f;
    vehicle->turnProgress = 0.0f;

    // 30% chance to turn
    int turnChance = rand() % 100;
    if (turnChance < 30)
    {
        vehicle->turnDirection = (turnChance < 15) ? TURN_LEFT : TURN_RIGHT;
    }
    else
    {
        vehicle->turnDirection = TURN_NONE;
    }

    // Set dimensions based on direction
    if (direction == DIRECTION_NORTH || direction == DIRECTION_SOUTH)
    {
        vehicle->rect.w = 20; // width
        vehicle->rect.h = 30; // height
    }
    else
    {
        vehicle->rect.w = 30; // width
        vehicle->rect.h = 20; // height
    }

    // Fixed spawn positions for each direction
    switch (direction)
    {
    case DIRECTION_NORTH:                             // Spawns at bottom, moves up
        vehicle->x = INTERSECTION_X - LANE_WIDTH / 2; // Left lane
        if (rand() % 2)
        { // Randomly choose right lane
            vehicle->x += LANE_WIDTH;
        }
        vehicle->y = WINDOW_HEIGHT - vehicle->rect.h;
        vehicle->isInRightLane = (vehicle->x > INTERSECTION_X);
        break;

    case DIRECTION_SOUTH:                             // Spawns at top, moves down
        vehicle->x = INTERSECTION_X - LANE_WIDTH / 2; // Left lane
        if (rand() % 2)
        { // Randomly choose right lane
            vehicle->x += LANE_WIDTH;
        }
        vehicle->y = 0;
        vehicle->isInRightLane = (vehicle->x > INTERSECTION_X);
        break;

    case DIRECTION_EAST: // Spawns at left, moves right
        vehicle->x = 0;
        vehicle->y = INTERSECTION_Y - LANE_WIDTH / 2; // Top lane
        if (rand() % 2)
        { // Randomly choose bottom lane
            vehicle->y += LANE_WIDTH;
        }
        vehicle->isInRightLane = (vehicle->y > INTERSECTION_Y);
        break;

    case DIRECTION_WEST: // Spawns at right, moves left
        vehicle->x = WINDOW_WIDTH - vehicle->rect.w;
        vehicle->y = INTERSECTION_Y - LANE_WIDTH / 2; // Top lane
        if (rand() % 2)
        { // Randomly choose bottom lane
            vehicle->y += LANE_WIDTH;
        }
        vehicle->isInRightLane = (vehicle->y > INTERSECTION_Y);
        break;
    }

    // Center vehicle in lane
    if (direction == DIRECTION_NORTH || direction == DIRECTION_SOUTH)
    {
        vehicle->x += (LANE_WIDTH / 4 - vehicle->rect.w / 2); // Center in lane
    }
    else
    {
        vehicle->y += (LANE_WIDTH / 4 - vehicle->rect.h / 2); // Center in lane
    }

    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;

    return vehicle;
}
void updateVehicle(Vehicle *vehicle, TrafficLight *lights)
{
    if (!vehicle->active)
        return;

    float stopLine = 0;
    bool shouldStop = false;
    float stopDistance = 40.0f;
    float turnPoint = 0;
    bool hasEmergencyPriority = (vehicle->type != REGULAR_CAR);

    // Calculate stop line based on direction
    switch (vehicle->direction)
    {
    case DIRECTION_NORTH:
        stopLine = INTERSECTION_Y + LANE_WIDTH + 40;
        if (vehicle->turnDirection == TURN_LEFT)
        {
            turnPoint = INTERSECTION_Y - LANE_WIDTH / 4;
        }
        else if (vehicle->turnDirection == TURN_RIGHT)
        {
            turnPoint = INTERSECTION_Y + LANE_WIDTH / 4;
        }
        else
        {
            turnPoint = INTERSECTION_Y;
        }
        break;
    case DIRECTION_SOUTH:
        stopLine = INTERSECTION_Y - LANE_WIDTH - 40;
        if (vehicle->turnDirection == TURN_LEFT)
        {
            turnPoint = INTERSECTION_Y + LANE_WIDTH / 4;
        }
        else if (vehicle->turnDirection == TURN_RIGHT)
        {
            turnPoint = INTERSECTION_Y - LANE_WIDTH / 4;
        }
        else
        {
            turnPoint = INTERSECTION_Y;
        }
        break;
    case DIRECTION_EAST:
        stopLine = INTERSECTION_X - LANE_WIDTH - 40;
        if (vehicle->turnDirection == TURN_LEFT)
        {
            turnPoint = INTERSECTION_X + LANE_WIDTH / 4;
        }
        else if (vehicle->turnDirection == TURN_RIGHT)
        {
            turnPoint = INTERSECTION_X - LANE_WIDTH / 4;
        }
        else
        {
            turnPoint = INTERSECTION_X;
        }
        break;
    case DIRECTION_WEST:
        stopLine = INTERSECTION_X + LANE_WIDTH + 40;
        if (vehicle->turnDirection == TURN_LEFT)
        {
            turnPoint = INTERSECTION_X - LANE_WIDTH / 4;
        }
        else if (vehicle->turnDirection == TURN_RIGHT)
        {
            turnPoint = INTERSECTION_X + LANE_WIDTH / 4;
        }
        else
        {
            turnPoint = INTERSECTION_X;
        }
        break;
    }

    // Check if vehicle should stop based on traffic lights
    if (!hasEmergencyPriority)
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            shouldStop = (vehicle->y > stopLine - stopDistance) &&
                         (vehicle->y < stopLine) &&
                         lights[DIRECTION_NORTH].state == RED;
            break;
        case DIRECTION_SOUTH:
            shouldStop = (vehicle->y < stopLine + stopDistance) &&
                         (vehicle->y > stopLine) &&
                         lights[DIRECTION_SOUTH].state == RED;
            break;
        case DIRECTION_EAST:
            shouldStop = (vehicle->x < stopLine + stopDistance) &&
                         (vehicle->x > stopLine) &&
                         lights[DIRECTION_EAST].state == RED;
            break;
        case DIRECTION_WEST:
            shouldStop = (vehicle->x > stopLine - stopDistance) &&
                         (vehicle->x < stopLine) &&
                         lights[DIRECTION_WEST].state == RED;
            break;
        }
    }
