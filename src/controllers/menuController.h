#pragma once
#include "lcom/lcf.h"
#include "../States/states.h"
#include "../model/menu.h"
/**
 * @brief Handles the menu controller
 * 
 * @param menu Menu to be handled
 * @param playerName Name of the player
 * @param interruptType Type of interrupt
 * @param packet Packet to be handled
 * @param scanCode Scan code to be handled
 * @param elapsed Time elapsed
 * @return STATE Next state
 
*/

STATE menuControllerHandle(Menu* menu,char* playerName, DEVICE interruptType, const struct packet *packet, uint8_t scanCode, unsigned elapsed);
