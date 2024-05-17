#ifndef TVPORT_PARAMETERS_HPP
#define TVPORT_PARAMETERS_HPP

#define TVPORT_DEFAULT_PORT_NUMBER 80
#define PREEXISTING_SLOT_NUMBER 0
#define TVPORT_MINIMUM_SLOT_NUMBER 1
#define TVPORT_MAXIMUM_SLOT_NUMBER 2
class ParamUtils {
    inline static const char* parameterSlotFileName = "slot.txt";
    inline static const char* parameterPortFileName = "port_number.txt";
public:

    static int readIntegerFromBuffer(char* buffer)
    {
        int r = 0;
        char c;
        while ((c = *buffer++) >= '0' && c <= '9')
        {
            r = r * 10 + c - 48;
        }
        return r;
    }

    static int readParameterInteger(char* fileName, int defValue)
    {
        FILE* fp;
        errno_t err;
        err = fopen_s(&fp, fileName, "r");

        if (err != 0 || fp == NULL)
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

    static void writeParameterInteger(char* fileName, int value)
    {
        FILE* fp;
        errno_t err = fopen_s(&fp, fileName, "w");

        if (err != 0 || fp == NULL)
        {
            std::cout << "Cannot write to " << fileName << std::endl;
            return;
        }

        // reading line by line, max 20 bytes
        const unsigned MAX_LENGTH = 20;
        char buffer[MAX_LENGTH];
        sprintf_s(buffer, "%d\n", value);
        fputs(buffer, fp);
        // close the file
        fclose(fp);
    }


    static void writeParameterSlot(int slot)
    {
        writeParameterInteger((char*)parameterSlotFileName, slot);
    }

    static int readParameterSlot()
    {
        int res = readParameterInteger((char*)parameterSlotFileName, 0);
        if (res < TVPORT_MINIMUM_SLOT_NUMBER || res>TVPORT_MAXIMUM_SLOT_NUMBER)
        {
            res = TVPORT_MINIMUM_SLOT_NUMBER;
            writeParameterSlot(res);
        }
        return res;
    }

    static void writeParameterPortNumber(int portNumber)
    {
        writeParameterInteger((char*)parameterPortFileName, portNumber);
    }

    static int readParameterPortNumber()
    {
        int res = readParameterInteger((char*)parameterPortFileName, 0);
        if (res <= 0)
        {
            res = TVPORT_DEFAULT_PORT_NUMBER;
            writeParameterPortNumber(res);
        }
        return res;
    }

};


#endif