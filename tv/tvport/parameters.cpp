#include <stdio.h>
#include <iostream>
#include "parameters.hpp"

using namespace std;

const char* parameterSlotFileName = "slot.txt";
const char* parameterPortFileName = "port_number.txt";


int readIntegerFromBuffer(char* buffer) 
{
    int r = 0;
    char c;
    while ((c = *buffer++) >= '0' && c <= '9') 
    {
        r = r * 10 + c - 48;
    }
    return r;
}

int readParameterInteger(char *fileName, int defValue)
{
    FILE* fp = fopen(fileName, "r");

    if (fp == NULL)
    {
        return defValue;
    }

    // reading line by line, max 256 bytes
    const unsigned MAX_LENGTH = 256;
    char buffer[MAX_LENGTH];

    int res = defValue;

    while (fgets(buffer, MAX_LENGTH, fp)) 
    {
        if (buffer[0] >= '0' && buffer[0] <= '9') 
        {
            res = readIntegerFromBuffer(buffer);
            break;
        }
    }
    // close the file
    fclose(fp);

    return res;
}

void writeParameterInteger(char* fileName, int value)
{
    FILE* fp = fopen(fileName, "w");

    if (fp == NULL)
    {
        cout << "Cannot write to " << fileName << endl;
        return;
    }

    // reading line by line, max 20 bytes
    const unsigned MAX_LENGTH = 20;
    char buffer[MAX_LENGTH];
    sprintf(buffer, "%d\n", value);
    fputs(buffer, fp);
    // close the file
    fclose(fp);
}


void writeParameterSlot(int slot)
{
    writeParameterInteger((char*)parameterSlotFileName, slot);
}

int readParameterSlot()
{
    int res = readParameterInteger((char*)parameterSlotFileName, 0);
    if (res < TVPORT_MINIMUM_SLOT_NUMBER || res>TVPORT_MAXIMUM_SLOT_NUMBER)
    {
        res = TVPORT_MINIMUM_SLOT_NUMBER;
        writeParameterSlot(res);
    }
    return res;
}

void writeParameterPortNumber(int portNumber)
{
    writeParameterInteger((char*)parameterPortFileName, portNumber);
}

int readParameterPortNumber()
{
    int res = readParameterInteger((char*)parameterPortFileName, 0);
    if (res <= 0)
    {
        res = TVPORT_DEFAULT_PORT_NUMBER;
        writeParameterPortNumber(res);
    }
    return res;
}
