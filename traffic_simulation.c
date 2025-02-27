#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "traffic_simulation.h"

// Global queues for lanes
Queue laneQueues[4];         // Queues for lanes A, B, C, D
int lanePriorities[4] = {0}; // Priority levels for lanes (0 = normal, 1 = high)

// Updated modern color scheme for vehicles
const SDL_Color VEHICLE_COLORS[] = {
    {60, 60, 70, 255},       // REGULAR_CAR: Dark slate gray
    {240, 240, 255, 255},    // AMBULANCE: Bright white with blue tint
    {30, 50, 140, 255},      // POLICE_CAR: Dark navy blue
    {220, 60, 10, 255}       // FIRE_TRUCK: Bright red-orange
};

// Road and UI color scheme
const SDL_Color ROAD_COLOR = {40, 40, 45, 255};      // Darker asphalt
const SDL_Color GRASS_COLOR = {60, 150, 80, 255};    // Grass green
const SDL_Color LANE_DIVIDER_COLOR = {240, 240, 200, 255}; // Off-white/yellow lane markers
const SDL_Color STOP_LINE_COLOR = {255, 255, 255, 255};    // White stop lines
const SDL_Color BACKGROUND_COLOR = {100, 180, 130, 255};   // Light green background (grass/terrain)
const SDL_Color QUEUE_FRAME_COLOR = {50, 50, 60, 200};     // Dark frame for queue display
const SDL_Color QUEUE_BG_COLOR = {220, 220, 220, 180};     // Light gray semi-transparent background

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

    // Update vehicle state based on stopping conditions
    if (shouldStop)
    {
        vehicle->state = STATE_STOPPING;
        vehicle->speed *= 0.8f; // Increased deceleration
        if (vehicle->speed < 0.1f)
        {
            vehicle->state = STATE_STOPPED;
            vehicle->speed = 0;
        }
    }
    else if (vehicle->state == STATE_STOPPED && !shouldStop)
    {
        vehicle->state = STATE_MOVING;
        // Reset speed based on vehicle type
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
    }

    // Decrease speed as vehicle approaches turn point
    if (vehicle->state == STATE_MOVING && vehicle->turnDirection != TURN_NONE)
    {
        float distanceToTurnPoint = 0;
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
        case DIRECTION_SOUTH:
            distanceToTurnPoint = fabs(vehicle->y - turnPoint);
            break;
        case DIRECTION_EAST:
        case DIRECTION_WEST:
            distanceToTurnPoint = fabs(vehicle->x - turnPoint);
            break;
        }

        if (distanceToTurnPoint < stopDistance)
        {
            vehicle->speed *= 1.0f;
            if (vehicle->speed < 0.5f)
            {
                vehicle->speed = 0.5f;
            }
        }
    }

    // Check if at turning point
    bool atTurnPoint = false;
    switch (vehicle->direction)
    {
    case DIRECTION_NORTH:
        atTurnPoint = vehicle->y <= turnPoint;
        break;
    case DIRECTION_SOUTH:
        atTurnPoint = vehicle->y >= turnPoint;
        break;
    case DIRECTION_EAST:
        atTurnPoint = vehicle->x >= turnPoint;
        break;
    case DIRECTION_WEST:
        atTurnPoint = vehicle->x <= turnPoint;
        break;
    }

    // Start turning if at turn point
    if (atTurnPoint && vehicle->turnDirection != TURN_NONE &&
        vehicle->state != STATE_TURNING && vehicle->state != STATE_STOPPED)
    {
        vehicle->state = STATE_TURNING;
        vehicle->turnAngle = 0.0f;
        vehicle->turnProgress = 0.0f;
    }

    // Movement logic
    float moveSpeed = vehicle->speed;
    if (vehicle->state == STATE_MOVING || vehicle->state == STATE_STOPPING)
    {
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            vehicle->y -= moveSpeed;
            break;
        case DIRECTION_SOUTH:
            vehicle->y += moveSpeed;
            break;
        case DIRECTION_EAST:
            vehicle->x += moveSpeed;
            break;
        case DIRECTION_WEST:
            vehicle->x -= moveSpeed;
            break;
        }
    }
    else if (vehicle->state == STATE_TURNING)
    {
        // Calculate turn angle based on vehicle type
        float turnSpeed = 1.0f;
        switch (vehicle->type)
        {
        case AMBULANCE:
        case POLICE_CAR:
            turnSpeed = 2.0f;
            break;
        case FIRE_TRUCK:
            turnSpeed = 1.5f;
            break;
        default:
            turnSpeed = 1.0f;
        }

        vehicle->turnAngle += turnSpeed;
        vehicle->turnProgress = vehicle->turnAngle / 90.0f;
        if (vehicle->turnAngle >= 90.0f)
        {
            vehicle->state = STATE_MOVING;
            vehicle->turnAngle = 0.0f;
            vehicle->turnProgress = 0.0f;
            vehicle->isInRightLane = !vehicle->isInRightLane;
        }

        // Calculate new position based on turn angle
        float turnRadius = 0.5f;
        float turnCenterX = 0;
        float turnCenterY = 0;
        float turnCenter = 15;
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            turnCenterX = vehicle->x + (vehicle->isInRightLane ? turnCenter : -turnCenter);
            turnCenterY = vehicle->y;
            ;
            break;
        case DIRECTION_SOUTH:
            turnCenterX = vehicle->x + (vehicle->isInRightLane ? -turnCenter : turnCenter);
            turnCenterY = vehicle->y;
            break;
        case DIRECTION_EAST:
            turnCenterX = vehicle->x;
            turnCenterY = vehicle->y + (!vehicle->isInRightLane ? turnCenter : -turnCenter);
            break;
        case DIRECTION_WEST:
            turnCenterX = vehicle->x;
            turnCenterY = vehicle->y + (!vehicle->isInRightLane ? -turnCenter : turnCenter);
            break;
        }

        float radians = vehicle->turnAngle * M_PI / 180.0f;
        switch (vehicle->direction)
        {
        case DIRECTION_NORTH:
            vehicle->x = turnCenterX + turnRadius * sin(radians);
            vehicle->y = turnCenterY - turnRadius * cos(radians);
            break;
        case DIRECTION_SOUTH:
            vehicle->x = turnCenterX - turnRadius * sin(radians);
            vehicle->y = turnCenterY + turnRadius * cos(radians);
            break;
        case DIRECTION_EAST:
            vehicle->x = turnCenterX + turnRadius * cos(radians);
            vehicle->y = turnCenterY + turnRadius * sin(radians);
            break;
        case DIRECTION_WEST:
            vehicle->x = turnCenterX - turnRadius * cos(radians);
            vehicle->y = turnCenterY - turnRadius * sin(radians);
            break;
        }
    }

    // Update rectangle position
    vehicle->rect.x = (int)vehicle->x;
    vehicle->rect.y = (int)vehicle->y;

    // Check if vehicle has left the screen
    if (vehicle->x < -100 || vehicle->x > WINDOW_WIDTH + 100 ||
        vehicle->y < -100 || vehicle->y > WINDOW_HEIGHT + 100)
    {
        vehicle->active = false;
    }
}

// Enhanced road rendering with texture effect
void renderRoads(SDL_Renderer *renderer)
{
    // First draw the background (grass/terrain)
    SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
    SDL_Rect backgroundRect = {0, 0, WINDOW_WIDTH, WINDOW_HEIGHT};
    SDL_RenderFillRect(renderer, &backgroundRect);
    
    // Draw the asphalt roads
    SDL_SetRenderDrawColor(renderer, ROAD_COLOR.r, ROAD_COLOR.g, ROAD_COLOR.b, ROAD_COLOR.a);

    // Draw the intersection
    SDL_Rect intersection = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, LANE_WIDTH * 2, LANE_WIDTH * 2};
    SDL_RenderFillRect(renderer, &intersection);

    // Draw main roads
    SDL_Rect verticalRoad1 = {INTERSECTION_X - LANE_WIDTH, 0, LANE_WIDTH * 2, INTERSECTION_Y - LANE_WIDTH};
    SDL_Rect verticalRoad2 = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH, LANE_WIDTH * 2, WINDOW_HEIGHT - INTERSECTION_Y - LANE_WIDTH};
    SDL_Rect horizontalRoad1 = {0, INTERSECTION_Y - LANE_WIDTH, INTERSECTION_X - LANE_WIDTH, LANE_WIDTH * 2};
    SDL_Rect horizontalRoad2 = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, WINDOW_WIDTH - INTERSECTION_X - LANE_WIDTH, LANE_WIDTH * 2};
    SDL_RenderFillRect(renderer, &verticalRoad1);
    SDL_RenderFillRect(renderer, &verticalRoad2);
    SDL_RenderFillRect(renderer, &horizontalRoad1);
    SDL_RenderFillRect(renderer, &horizontalRoad2);

    // Add road texture effect - simple grid pattern
    SDL_SetRenderDrawColor(renderer, 50, 50, 55, 50); // Slightly lighter than the road
    for (int x = 0; x < WINDOW_WIDTH; x += 20) {
        for (int y = 0; y < WINDOW_HEIGHT; y += 20) {
            // Only add texture on road areas
            if ((x >= INTERSECTION_X - LANE_WIDTH && x <= INTERSECTION_X + LANE_WIDTH) ||
                (y >= INTERSECTION_Y - LANE_WIDTH && y <= INTERSECTION_Y + LANE_WIDTH)) {
                SDL_Rect textureRect = {x, y, 2, 2};
                SDL_RenderFillRect(renderer, &textureRect);
            }
        }
    }

    // Draw lane dividers - make them more visible
    SDL_SetRenderDrawColor(renderer, LANE_DIVIDER_COLOR.r, LANE_DIVIDER_COLOR.g, LANE_DIVIDER_COLOR.b, LANE_DIVIDER_COLOR.a);
    for (int i = 0; i < WINDOW_HEIGHT; i += 40)
    {
        if (i < INTERSECTION_Y - LANE_WIDTH || i > INTERSECTION_Y + LANE_WIDTH)
        {
            SDL_Rect laneDivider1 = {INTERSECTION_X - LANE_WIDTH / 2 - 1, i, 3, 25}; // Wider and longer dividers
            SDL_Rect laneDivider2 = {INTERSECTION_X + LANE_WIDTH / 2 - 1, i, 3, 25};
            SDL_RenderFillRect(renderer, &laneDivider1);
            SDL_RenderFillRect(renderer, &laneDivider2);
        }
    }
    for (int i = 0; i < WINDOW_WIDTH; i += 40)
    {
        if (i < INTERSECTION_X - LANE_WIDTH || i > INTERSECTION_X + LANE_WIDTH)
        {
            SDL_Rect laneDivider1 = {i, INTERSECTION_Y - LANE_WIDTH / 2 - 1, 25, 3};
            SDL_Rect laneDivider2 = {i, INTERSECTION_Y + LANE_WIDTH / 2 - 1, 25, 3};
            SDL_RenderFillRect(renderer, &laneDivider1);
            SDL_RenderFillRect(renderer, &laneDivider2);
        }
    }

    // Add stop lines with enhanced visibility
    SDL_SetRenderDrawColor(renderer, STOP_LINE_COLOR.r, STOP_LINE_COLOR.g, STOP_LINE_COLOR.b, STOP_LINE_COLOR.a);
    
    // Make stop lines thicker for better visibility
    int stopLineThickness = STOP_LINE_WIDTH * 2;
    
    SDL_Rect northStop = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH - stopLineThickness, LANE_WIDTH * 2, stopLineThickness};
    SDL_Rect southStop = {INTERSECTION_X - LANE_WIDTH, INTERSECTION_Y + LANE_WIDTH, LANE_WIDTH * 2, stopLineThickness};
    SDL_Rect eastStop = {INTERSECTION_X + LANE_WIDTH, INTERSECTION_Y - LANE_WIDTH, stopLineThickness, LANE_WIDTH * 2};
    SDL_Rect westStop = {INTERSECTION_X - LANE_WIDTH - stopLineThickness, INTERSECTION_Y - LANE_WIDTH, stopLineThickness, LANE_WIDTH * 2};
    
    SDL_RenderFillRect(renderer, &northStop);
    SDL_RenderFillRect(renderer, &southStop);
    SDL_RenderFillRect(renderer, &eastStop);
    SDL_RenderFillRect(renderer, &westStop);
}

// Queue visualization (hidden)
void renderQueues(SDL_Renderer *renderer)
{
    // This function now keeps track of queue data internally
    // but doesn't render any visual elements to the screen
    
    // Note: We still maintain the queue data structure for traffic flow logic
    // but we're not displaying it as requested by the user
}

// Enhanced traffic light rendering
void renderTrafficLight(SDL_Renderer *renderer, TrafficLight *light)
{
    // Draw traffic light housing with a more 3D look
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255); // Dark gray base
    SDL_RenderFillRect(renderer, &light->position);
    
    // Add lighter edge on top/left for 3D effect
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    if (light->direction == DIRECTION_NORTH || light->direction == DIRECTION_SOUTH) {
        SDL_Rect topEdge = {light->position.x, light->position.y, light->position.w, 2};
        SDL_Rect leftEdge = {light->position.x, light->position.y, 2, light->position.h};
        SDL_RenderFillRect(renderer, &topEdge);
        SDL_RenderFillRect(renderer, &leftEdge);
    } else {
        SDL_Rect topEdge = {light->position.x, light->position.y, light->position.h, 2};
        SDL_Rect leftEdge = {light->position.x, light->position.y, 2, light->position.w};
        SDL_RenderFillRect(renderer, &topEdge);
        SDL_RenderFillRect(renderer, &leftEdge);
    }
    
    // Draw the light itself with a glow effect
    int padding = 4;
    SDL_Rect lightRect = {
        light->position.x + padding,
        light->position.y + padding,
        light->position.w - padding * 2,
        light->position.h - padding * 2
    };
    
    // Light color with full brightness
    SDL_SetRenderDrawColor(renderer, 
        light->state == RED ? 255 : 40,   // R
        light->state == GREEN ? 255 : 40, // G
        40,                               // B
        255);                             // A
    
    SDL_RenderFillRect(renderer, &lightRect);
    
    // Add a glow effect (lighter center)
    int innerPadding = padding + 3;
    SDL_Rect glowRect = {
        light->position.x + innerPadding,
        light->position.y + innerPadding,
        light->position.w - innerPadding * 2,
        light->position.h - innerPadding * 2
    };
    
    SDL_SetRenderDrawColor(renderer, 
        light->state == RED ? 255 : 80,   // R 
        light->state == GREEN ? 255 : 80, // G
        80,                               // B
        255);                             // A
    
    SDL_RenderFillRect(renderer, &glowRect);
}

// Enhanced vehicle rendering
void renderVehicle(SDL_Renderer *renderer, Vehicle *vehicle)
{
    if (!vehicle->active)
        return;
    
    // Base vehicle color
    SDL_Color baseColor = VEHICLE_COLORS[vehicle->type];
    
    // Draw the main vehicle body
    SDL_SetRenderDrawColor(renderer, baseColor.r, baseColor.g, baseColor.b, baseColor.a);
    SDL_RenderFillRect(renderer, &vehicle->rect);
    
    // Add vehicle details based on type
    int detailPadding = 2;
    SDL_Rect detailRect = {
        vehicle->rect.x + detailPadding,
        vehicle->rect.y + detailPadding,
        vehicle->rect.w - detailPadding * 2,
        vehicle->rect.h - detailPadding * 2
    };
    
    // Add vehicle type-specific details
    switch (vehicle->type) {
        case AMBULANCE:
            // White cross for ambulance
            SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
            if (vehicle->direction == DIRECTION_NORTH || vehicle->direction == DIRECTION_SOUTH) {
                int crossW = vehicle->rect.w / 3;
                int crossH = vehicle->rect.h / 2;
                int crossX = vehicle->rect.x + (vehicle->rect.w - crossW) / 2;
                int crossY = vehicle->rect.y + (vehicle->rect.h - crossH) / 2;
                
                SDL_Rect verticalBar = {
                    crossX + crossW/2 - 2,
                    crossY,
                    4,
                    crossH
                };
                SDL_Rect horizontalBar = {
                    crossX,
                    crossY + crossH/2 - 2,
                    crossW,
                    4
                };
                SDL_RenderFillRect(renderer, &verticalBar);
                SDL_RenderFillRect(renderer, &horizontalBar);
            } else {
                int crossW = vehicle->rect.w / 2;
                int crossH = vehicle->rect.h / 3;
                int crossX = vehicle->rect.x + (vehicle->rect.w - crossW) / 2;
                int crossY = vehicle->rect.y + (vehicle->rect.h - crossH) / 2;
                
                SDL_Rect verticalBar = {
                    crossX + crossW/2 - 2,
                    crossY,
                    4,
                    crossH
                };
                SDL_Rect horizontalBar = {
                    crossX,
                    crossY + crossH/2 - 2,
                    crossW,
                    4
                };
                SDL_RenderFillRect(renderer, &verticalBar);
                SDL_RenderFillRect(renderer, &horizontalBar);
            }
            break;
            
        case POLICE_CAR:
            // Blue/red stripe for police car
            if (vehicle->direction == DIRECTION_NORTH || vehicle->direction == DIRECTION_SOUTH) {
                SDL_Rect redStripe = {
                    vehicle->rect.x,
                    vehicle->rect.y + vehicle->rect.h / 4,
                    vehicle->rect.w,
                    vehicle->rect.h / 8
                };
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &redStripe);
                
                SDL_Rect blueStripe = {
                    vehicle->rect.x,
                    vehicle->rect.y + vehicle->rect.h / 4 + vehicle->rect.h / 8,
                    vehicle->rect.w,
                    vehicle->rect.h / 8
                };
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderFillRect(renderer, &blueStripe);
            } else {
                SDL_Rect redStripe = {
                    vehicle->rect.x + vehicle->rect.w / 4,
                    vehicle->rect.y,
                    vehicle->rect.w / 8,
                    vehicle->rect.h
                };
                SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
                SDL_RenderFillRect(renderer, &redStripe);
                
                SDL_Rect blueStripe = {
                    vehicle->rect.x + vehicle->rect.w / 4 + vehicle->rect.w / 8,
                    vehicle->rect.y,
                    vehicle->rect.w / 8,
                    vehicle->rect.h
                };
                SDL_SetRenderDrawColor(renderer, 0, 0, 255, 255);
                SDL_RenderFillRect(renderer, &blueStripe);
            }
            break;
            
        case FIRE_TRUCK:
            // Yellow stripe for fire truck
            if (vehicle->direction == DIRECTION_NORTH || vehicle->direction == DIRECTION_SOUTH) {
                SDL_Rect stripe = {
                    vehicle->rect.x,
                    vehicle->rect.y + vehicle->rect.h / 3,
                    vehicle->rect.w,
                    vehicle->rect.h / 6
                };
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderFillRect(renderer, &stripe);
            } else {
                SDL_Rect stripe = {
                    vehicle->rect.x + vehicle->rect.w / 3,
                    vehicle->rect.y,
                    vehicle->rect.w / 6,
                    vehicle->rect.h
                };
                SDL_SetRenderDrawColor(renderer, 255, 255, 0, 255);
                SDL_RenderFillRect(renderer, &stripe);
            }
            break;
            
        case REGULAR_CAR:
            // Add windows to regular car
            SDL_SetRenderDrawColor(renderer, 180, 210, 240, 255); // Light blue windows
            
            if (vehicle->direction == DIRECTION_NORTH || vehicle->direction == DIRECTION_SOUTH) {
                SDL_Rect windshield = {
                    vehicle->rect.x + 3,
                    vehicle->rect.y + 3,
                    vehicle->rect.w - 6,
                    vehicle->rect.h / 3
                };
                SDL_RenderFillRect(renderer, &windshield);
                
                SDL_Rect rearWindow = {
                    vehicle->rect.x + 3,
                    vehicle->rect.y + vehicle->rect.h - vehicle->rect.h / 3 - 3,
                    vehicle->rect.w - 6,
                    vehicle->rect.h / 3 - 3
                };
                SDL_RenderFillRect(renderer, &rearWindow);
            } else {
                SDL_Rect windshield = {
                    vehicle->rect.x + 3,
                    vehicle->rect.y + 3,
                    vehicle->rect.w / 3,
                    vehicle->rect.h - 6
                };
                SDL_RenderFillRect(renderer, &windshield);
                
                SDL_Rect rearWindow = {
                    vehicle->rect.x + vehicle->rect.w - vehicle->rect.w / 3 - 3,
                    vehicle->rect.y + 3,
                    vehicle->rect.w / 3 - 3,
                    vehicle->rect.h - 6
                };
                SDL_RenderFillRect(renderer, &rearWindow);
            }
            break;
    }
    
    // Add 3D effect with a darker shadow on the bottom/right
    SDL_SetRenderDrawColor(renderer, 
        baseColor.r > 50 ? baseColor.r - 50 : 0, 
        baseColor.g > 50 ? baseColor.g - 50 : 0, 
        baseColor.b > 50 ? baseColor.b - 50 : 0, 
        baseColor.a);
    
    SDL_Rect bottomEdge = {
        vehicle->rect.x,
        vehicle->rect.y + vehicle->rect.h - 2,
        vehicle->rect.w,
        2
    };
    SDL_Rect rightEdge = {
        vehicle->rect.x + vehicle->rect.w - 2,
        vehicle->rect.y,
        2,
        vehicle->rect.h
    };
    
    SDL_RenderFillRect(renderer, &bottomEdge);
    SDL_RenderFillRect(renderer, &rightEdge);
}

void renderSimulation(SDL_Renderer *renderer, Vehicle *vehicles, TrafficLight *lights, Statistics *stats)
{
    // Draw background
    SDL_SetRenderDrawColor(renderer, BACKGROUND_COLOR.r, BACKGROUND_COLOR.g, BACKGROUND_COLOR.b, BACKGROUND_COLOR.a);
    SDL_RenderClear(renderer);

    // Render roads with textures and markings
    renderRoads(renderer);

    // Render enhanced traffic lights
    for (int i = 0; i < 4; i++)
    {
        renderTrafficLight(renderer, &lights[i]);
    }

    // Render enhanced vehicles
    for (int i = 0; i < MAX_VEHICLES; i++)
    {
        if (vehicles[i].active)
        {
            renderVehicle(renderer, &vehicles[i]);
        }
    }

    // Render queue display
    renderQueues(renderer);

    // Present the rendered frame
    SDL_RenderPresent(renderer);
}

// Queue functions
void initQueue(Queue *q)
{
    q->front = q->rear = NULL;
    q->size = 0;
}

void enqueue(Queue *q, Vehicle vehicle)
{
    Node *newNode = (Node *)malloc(sizeof(Node));
    newNode->vehicle = vehicle;
    newNode->next = NULL;
    if (q->rear == NULL)
    {
        q->front = q->rear = newNode;
    }
    else
    {
        q->rear->next = newNode;
        q->rear = newNode;
    }
    q->size++;
}

Vehicle dequeue(Queue *q)
{
    if (q->front == NULL)
    {
        Vehicle emptyVehicle = {0};
        return emptyVehicle;
    }
    Node *temp = q->front;
    Vehicle vehicle = temp->vehicle;
    q->front = q->front->next;
    if (q->front == NULL)
    {
        q->rear = NULL;
    }
    free(temp);
    q->size--;
    return vehicle;
}

int isQueueEmpty(Queue *q)
{
    return q->front == NULL;
}