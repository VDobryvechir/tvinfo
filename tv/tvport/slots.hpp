/*************************************************************
We start with slot 1, then go to next slots and go round. 
We check the consistency of the slot, if the config is present, and all image and video files are ready, we start working with this slot.
When the slot is ready, we remove all files that are extra in that slot and indicate to the tvport to come to the next slot
When we load the config, we check its consistency and availability of files. If they are available, we start working with them.
If not, we return the information to the main server.
The main server can upload files using a special function, which receives the files, portionOffset and amount, and when it is done,
we again check for consistency. When it is done, we show the info to go to the next slot.
Name of each file is as follows
  first letter: 'i' or 'v'
  second letters: id= 0, 1, 2, ... for pictures
             file name without extension for video
  next letter: '_'
  next digits: check sum
  next letter: '-'
  next digits: file length
  next letters: .extension (.jpg, .mp4, .png, ...)

  information about absense is given in the format as follows:
   {"fileName1":offset1, "fileName2":offset2}, where offset is the number of available bytes

TvPortSlot provide all functionality necessary for the current slot

TvPortSlots manages the current slot for the show and next slot for uploading in parallel,
   also it manages the initial loading of the slot from the file system

**************************************************************/


#ifndef TVPORT_SLOTS_HPP
#define TVPORT_SLOTS_HPP

#include <filesystem>
#include <iostream> 
#include <fstream>
#include <vector> 
#include <map>
#include <string>

#include <boost/json.hpp>

#include "parameters.hpp"

using namespace std;
namespace json = boost::json;

struct TvPortConfiguration {
	vector<string> file;
	vector<int> duration;
};

class TvPortSlot
{
	const string configFilePath = "config.json";
public:
	int slotNumber;
	int screenNumber=0;
	bool isReady = false, isCorrupted=false;
	string pathPrefix;
	string reason;

	vector<string> file;
	vector<int> duration;
	map<string, int> problems;

	TvPortSlot(int slot)
	{
		slotNumber = slot;
		pathPrefix = to_string(slot);
		if (!filesystem::exists(pathPrefix)) {
			filesystem::create_directory(pathPrefix);
		}
		pathPrefix += "/";
	}

	bool readConfigFile(string path)
	{
		ifstream ifs(path);
		string input(istreambuf_iterator<char>(ifs), {});

		TvPortConfiguration conf = json::value_to<TvPortConfiguration>(json::parse(input));
		file = conf.file;
		duration = conf.duration;
	}

	bool readSlot()
	{
		isCorrupted = false;
		isReady = false;
		string configName = pathPrefix + configFilePath;
		if (!filesystem::exists(configName))
		{
			isCorrupted = true;
			reason = "File " + configName + " does not exist";
			return false;
		}
		if (!readConfigFile(configName))
		{
			isCorrupted = true;
			reason = "Corrupted config file context";
			return false;
		}
		return verifySlot();
	}

	bool verifySlot()
	{
		isReady = false;
		int n = file.size();
		if (n == 0 || duration.size() != n)
		{
			isCorrupted = true;
			reason = "Incorrect size of file or duration";
			return false;
		}
		problems.clear();
		isReady = true;
		for (int i = 0; i < n; i++)
		{
			int varighet = duration.at(i);
			string fil = file.at(i);
			if (fil.size() < 4 || (fil.at(0) != 'v' && fil.at(0) != 'i'))
			{
				isCorrupted = true;
				isReady = false;
				reason = "Incorrect file name start at " + to_string(i) + " of " + fil;
				return false;
			}
			if (varighet <= 0 && fil.at(0) == 'i')
			{
				isCorrupted = true;
				isReady = false;
				reason = "Incorrect duration at " + to_string(i) + " of " + to_string(varighet);
				return false;
			}
			long sizeExpected = getExpectedFileSize(i);
			if (sizeExpected <= 0)
			{
				return false;
			}
			string filName = pathPrefix + fil;
			if (!filesystem::exists(filName))
			{
				isReady = false;
				problems[filName] = 0;
			}
			else
			{
				uintmax_t fileSize = filesystem::file_size(filName);
				if (fileSize != (uintmax_t)sizeExpected) {
					isReady = false;
					problems[filName] = (int)fileSize;
				}
			}
		}
		return isReady;
	}

	string uploadConfig(string configData)
	{
		ofstream conf(pathPrefix + configFilePath);
		if (conf.is_open()) {
			conf << configData;
			conf.close();
		}
		else {
			return "Unable to create config.json file.\n";
		}
		readSlot();
		return getCommonStatus();
	}

	string getCommonStatus() 
	{
		stringstream ss;
		if (isCorrupted)
		{
			ss << "{\"Error: " << reason << "\":0}";
		} 
		else if (isReady)
		{
			ss << "{}";
		} 
		else
		{
			bool first = true;
			for (auto const& [key, val] : problems)
			{
				ss << (first ? "{" : ",") << "\"" << key << "\":" << val;
				first = false;
			}
			ss << "}";
		}
		return ss.str();
	}

	long getExpectedFileSize(int pos)
	{
		string fil = file.at(pos);
		size_t minusPos = fil.find('-');
		size_t pointPos = fil.find('.');
		if (minusPos == string::npos || pointPos == string::npos || minusPos + 1 >= pointPos)
		{
			isCorrupted = true;
			isReady = false;
			reason = "Incorrect name structure with regard to minus or point in " + to_string(pos) + " of " + fil;
			return -1;
		}
		string sizeStr = fil.substr(minusPos + 1, pointPos - minusPos - 1);
		long sizeExpected;
		if (sscanf(sizeStr.c_str(), "%ld", &sizeExpected) != 1 || sizeExpected <= 0)
		{
			isCorrupted = true;
			isReady = false;
			reason = "Incorrect file size in file name " + fil;
			return -1;
		}
		return sizeExpected;
	}

	string saveSlotFile(int fileNo, long filePos, int storrelse, char* data)
	{
		string fileName = pathPrefix + file.at(fileNo);
		if (filePos == 0)
		{
			std::ofstream fs(fileName, std::ios::out | std::ios::binary | std::ios::app);
			fs.write(data, storrelse);
			fs.close();
		}
		else {
			if (!filesystem::exists(fileName))
			{
				return "File " + fileName + " does not exist, so it cannot be written at this position " + to_string(filePos);
			}
			uintmax_t fileSize = filesystem::file_size(fileName);
			if (fileSize < filePos)
			{
				return "File " + fileName + " is too small " + to_string(fileSize) + " so it cannot be saved at position " + to_string(filePos);
			}
			std::fstream fs(fileName, std::ios::binary | std::ios::out | std::ios::in);
			fs.seekp(filePos, std::ios::beg); // Move the write pointer to position 
			fs.write(data, storrelse); // Write data at position filePos
			fs.close();
		}
		return "";
	}

	// nr must be of this format X_XXXXXX, where X is the file number in the slot, XXXXXX is the current position in the file
	string uploadFile(string nr, int storrelse, char* data) {
		size_t underPos = nr.find("_");
		if (underPos == string::npos || underPos < 1)
		{
			return "Incorrect number parameter";
		}
		string firstNmb = nr.substr(0, underPos);
		string secondNmb = nr.substr(underPos + 1);
		int fileNo;
		long filePos;
		if (sscanf(firstNmb.c_str(), "%d", &fileNo) != 1 || fileNo < 0 || fileNo >= file.size())
		{
			return "Error in the file no  with limit of " + to_string(file.size());
		}
		if (sscanf(secondNmb.c_str(), "%ld", &filePos) != 1 || filePos < 0 || storrelse<=0)
		{
			return "Error in the file position of " + secondNmb;
		}
		long expectedSize = getExpectedFileSize(fileNo);
		if (expectedSize <= 0)
		{
			return "Corrupted expected size " + to_string(expectedSize);
		}
		if (expectedSize < filePos + storrelse)
		{
			return "Exceeded expected file size " + to_string(expectedSize) + " while filePos= " + to_string(filePos) + " size=" + to_string(size);
		}
		string message = saveSlotFile(fileNo, filePos, storrelse, data);
		if (message.size() > 0)
		{
			return message;
		}
		readSlot();
		return getCommonStatus();
	}

	void cleanUnnecessaryFiles(bool forceAll)
	{
		if (!forceAll && isCorrupted) {
			forceAll = true;
		}
		if (slotNumber==0) 
		{
			return;
		}
		if (forceAll) 
		{
			for (const auto& entry : filesystem::directory_iterator(pathPrefix))
			{
				if (entry.is_regular_file())
				{
					filesystem::remove_all(entry.path());
				}
			}
		}
		else {

		}
		// TODO
	}
};

class TvPortSlots
{
	int currentSlot;
	TvPortSlot *current = nullptr;
	TvPortSlot *next = nullptr;
public:
	TvPortSlots()
	{
		currentSlot = readParameterSlot();
		current = readSlot(currentSlot);
		if (!current->isReady || current->isCorrupted)
		{
			current->cleanUnnecessaryFiles(true);
			currentSlot = 0;
			delete current;
			current = readSlot(currentSlot);
		}
	}

	int getNextSlotNumber()
	{
		int slot = currentSlot + 1;
		if (slot > TVPORT_MAXIMUM_SLOT_NUMBER)
		{
			slot = TVPORT_MINIMUM_SLOT_NUMBER;
		}
		return slot;
	}

	void goToNextSlot()
	{
		currentSlot = getNextSlotNumber();
	}

protected:
	TvPortSlot *readSlot(int slot)
	{

	}
	void readNextSlot()
	{
		readSlot(getNextSlotNumber());
	}
	void readCurrentSlot()
	{
		readSlot(currentSlot);
	}
};

#endif