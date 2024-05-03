#ifndef TVPORT_PARAMETERS_HPP
#define TVPORT_PARAMETERS_HPP

#define TVPORT_DEFAULT_PORT_NUMBER 80
#define PREEXISTING_SLOT_NUMBER 0
#define TVPORT_MINIMUM_SLOT_NUMBER 1
#define TVPORT_MAXIMUM_SLOT_NUMBER 2

int readIntegerFromBuffer(char* buffer);
int readParameterInteger(char* fileName, int defValue);
void writeParameterInteger(char* fileName, int value);
void writeParameterSlot(int slot);
int readParameterSlot();
void writeParameterPortNumber(int portNumber);
int readParameterPortNumber();

#endif