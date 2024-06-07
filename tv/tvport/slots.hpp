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

namespace filesystem = std::filesystem;
namespace json = boost::json;

struct TvPortConfiguration {
	std::vector<std::string> file;
	std::vector<int> duration;

	friend TvPortConfiguration tag_invoke(json::value_to_tag<TvPortConfiguration>, json::value const& v) {
		auto& o = v.as_object();
		return {
			json::value_to<std::vector<std::string>>(o.at("file")),
			json::value_to<std::vector<int>>(o.at("duration")),
		};
	}

	friend void tag_invoke(json::value_from_tag, json::value& v, TvPortConfiguration const& rec)
	{
		auto file = json::value_from(rec.file);
		auto duration = json::value_from( rec.duration );

		v = json::object{
			{"file", file},
			{"duration", duration},
		};
	}
};

class TvPortSlot
{
	const std::string configFilePath = "config.json";
public:
	int slotNumber;
	bool isReady = false, isCorrupted=false, readyToSwitch=false;
	std::string pathPrefix;
	std::string reason;

	std::vector<std::string> file;
	std::vector<int> duration;
	std::map<std::string, int> problems;

	TvPortSlot(int slot)
	{
		slotNumber = slot;
		pathPrefix = std::to_string(slot);
		if (!filesystem::exists(pathPrefix)) {
			filesystem::create_directory(pathPrefix);
		}
		pathPrefix += "/";
	}

    int getScreenNumber() 
    {
              return isCorrupted || !isReady ? 0 : (int) file.size();
    }

	std::string	getSlotFileName(int screen)
	{
		return screen < file.size() ? pathPrefix + file.at(screen) : "";
	}

	int getSlotDuration(int screen)
	{
		int res = screen < duration.size() ? duration.at(screen) : 0;
		if (res<=0) 
		{
			res = 3;
		}
		return res;
	}

	std::string getAllDurations() {
		std::string res = "";
		int n = duration.size();
		for (int i = 0; i < n; i++)
		{
			if (i != 0) {
				res += ",";
			}
			res += std::to_string(duration.at(i));
		}
		return res;
	}

	std::string getAllFiles() {
		std::string res = "";
		int n = file.size();
		for (int i = 0; i < n; i++)
		{
			if (i != 0) {
				res += ",";
			}
			res += "\"/" + pathPrefix + file.at(i) + "\"";
		}
		return res;
	}

	bool filePathContainVideo(std::string name) {
		if (name.size() == 0) {
			return false;
		}
		size_t found = name.find_last_of('/');
		if (found == std::string::npos) {
			return name.at(0) == 'v';
		}
		if (found + 1 >= name.size()) {
			return false;
		}
		return name.at(found + 1) == 'v';
	}

	bool isSlotVideo(int screen)
	{
		return screen < file.size() && filePathContainVideo(file.at(screen));
	}

	bool readConfigFile(std::string path)
	{
		std::ifstream ifs(path);
		std::string input(std::istreambuf_iterator<char>(ifs), {});

		TvPortConfiguration conf = json::value_to<TvPortConfiguration>(json::parse(input));
		file = conf.file;
		duration = conf.duration;
		return !conf.file.empty();
	}

	bool readSlot()
	{
		isCorrupted = false;
		isReady = false;
		std::string configName = pathPrefix + configFilePath;
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
		int n = (int) file.size();
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
			std::string fil = file.at(i);
			if (fil.size() < 4 || (fil.at(0) != 'v' && fil.at(0) != 'i'))
			{
				isCorrupted = true;
				isReady = false;
				reason = "Incorrect file name start at " + std::to_string(i) + " of " + fil;
				return false;
			}
			if (varighet <= 0 && fil.at(0) == 'i')
			{
				isCorrupted = true;
				isReady = false;
				reason = "Incorrect duration at " + std::to_string(i) + " of " + std::to_string(varighet);
				return false;
			}
			long sizeExpected = getExpectedFileSize(i);
			if (sizeExpected <= 0)
			{
				return false;
			}
			std::string filName = pathPrefix + fil;
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

	std::string uploadConfig(std::string configData)
	{
		std::ofstream conf(pathPrefix + configFilePath);
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

	std::string getCommonStatus() 
	{
		std::stringstream ss;
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
		std::string fil = file.at(pos);
		size_t minusPos = fil.find('-');
		size_t pointPos = fil.find('.');
		if (minusPos == std::string::npos || pointPos == std::string::npos || minusPos + 1 >= pointPos)
		{
			isCorrupted = true;
			isReady = false;
			reason = "Incorrect name structure with regard to minus or point in " + std::to_string(pos) + " of " + fil;
			return -1;
		}
		std::string sizeStr = fil.substr(minusPos + 1, pointPos - minusPos - 1);
		long sizeExpected;
		if (sscanf_s(sizeStr.c_str(), "%ld", &sizeExpected) != 1 || sizeExpected <= 0)
		{
			isCorrupted = true;
			isReady = false;
			reason = "Incorrect file size in file name " + fil;
			return -1;
		}
		return sizeExpected;
	}

	std::string saveSlotFile(int fileNo, long filePos, int storrelse, char* data)
	{
		std::string fileName = pathPrefix + file.at(fileNo);
		if (filePos == 0)
		{
			std::ofstream fs(fileName, std::ios::out | std::ios::binary | std::ios::app);
			fs.write(data, storrelse);
			fs.close();
		}
		else {
			if (!filesystem::exists(fileName))
			{
				return "File " + fileName + " does not exist, so it cannot be written at this position " + std::to_string(filePos);
			}
			uintmax_t fileSize = filesystem::file_size(fileName);
			if (fileSize < filePos)
			{
				return "File " + fileName + " is too small " + std::to_string(fileSize) + " so it cannot be saved at position " + std::to_string(filePos);
			}
			std::fstream fs(fileName, std::ios::binary | std::ios::out | std::ios::in);
			fs.seekp(filePos, std::ios::beg); // Move the write pointer to position 
			fs.write(data, storrelse); // Write data at position filePos
			fs.close();
		}
		return "";
	}

	// nr must be of this format X_XXXXXX, where X is the file number in the slot, XXXXXX is the current position in the file
	std::string uploadFile(std::string nr, int uploadedSize, char* data) {
		size_t underPos = nr.find("_");
		if (underPos == std::string::npos || underPos < 1)
		{
			return "Incorrect number parameter";
		}
		std::string firstNmb = nr.substr(0, underPos);
		std::string secondNmb = nr.substr(underPos + 1);
		int fileNo;
		long filePos;
		if (sscanf_s(firstNmb.c_str(), "%d", &fileNo) != 1 || fileNo < 0 || fileNo >= file.size())
		{
			return "Error in the file no  with limit of " + std::to_string(file.size());
		}
		if (sscanf_s(secondNmb.c_str(), "%ld", &filePos) != 1 || filePos < 0 || uploadedSize<=0)
		{
			return "Error in the file position of " + secondNmb;
		}
		long expectedSize = getExpectedFileSize(fileNo);
		if (expectedSize <= 0)
		{
			return "Corrupted expected size " + std::to_string(expectedSize);
		}
		if (expectedSize < filePos + uploadedSize)
		{
			return "Exceeded expected file size " + std::to_string(expectedSize) + " while filePos= " + std::to_string(filePos) + " size=" + std::to_string(uploadedSize);
		}
		std::string message = saveSlotFile(fileNo, filePos, uploadedSize, data);
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
			for (const auto& entry : filesystem::directory_iterator(pathPrefix))
			{
				if (entry.is_regular_file())
				{
					filesystem::path p = entry.path();
					std::string s = p.filename().string();
					if (s == configFilePath || find(file.begin(), file.end(), s) != file.end())
					{
						continue;
					}
					filesystem::remove_all(p);
				}
			}
		}
	}
};

class TvPortSlots
{
	volatile int currentSlot;
	TvPortSlot *current = nullptr;
	TvPortSlot *next = nullptr;
	std::mutex switchToNextMutex;
public:
	TvPortSlots()
	{
		currentSlot = ParamUtils::readParameterSlot();
	}
	int loadInitialSlot() 
    {
		current = readSlot(currentSlot);
		if (!current->isReady || current->isCorrupted)
		{
			current->cleanUnnecessaryFiles(true);
			currentSlot = 0;
			delete current;
			current = readSlot(currentSlot);
		}
        return currentSlot;            
    }
	int getCurrentSlotNumber() {
		return currentSlot;
	}

	std::string getCurrentSlotFiles()
	{
		if (current == nullptr) {
			return "";
		}
		return current->getAllFiles();
	}

	std::string getCurrentSlotDurations()
	{
		if (current == nullptr) {
			return "";
		}
		return current->getAllDurations();
	}

	std::string getAllPaddings()
	{
		std::string r = std::to_string(ParamUtils::readParameterPaddingTop()) + "," + std::to_string(ParamUtils::readParameterPaddingRight()) + "," + std::to_string(ParamUtils::readParameterPaddingBottom()) + "," + std::to_string(ParamUtils::readParameterPaddingLeft());
		return r;
	}

    int getCurrentSlotScreens() 
    {
		return current == nullptr ? 0 : current->getScreenNumber();
    }

	bool isRequiredToSwitch(int slot) {
		return slot != currentSlot;
	}

	std::string getCurrentSlotFileName(int screen) 
	{
		return current == nullptr ? "" : current->getSlotFileName(screen);
	}

	int getCurrentSlotDuration(int screen)
	{
		return current == nullptr ? 3 : current->getSlotDuration(screen);
	}

	bool isCurrentSlotVideo(int screen) 
	{
		return current == nullptr ? false : current->isSlotVideo(screen);
	}

	int switchToCurrentTask()
	{
		TvPortSlot* old = nullptr;
		switchToNextMutex.lock();
		if (next != nullptr && next->readyToSwitch) {
			old = current;
			current = next;
			next = nullptr;
		}
		switchToNextMutex.unlock();
		if (old!=nullptr) 
		{
			if (old->isCorrupted)
			{
				old->cleanUnnecessaryFiles(true);
			}
			delete old;
		}
		if (current!=nullptr) {
			currentSlot = current->slotNumber;
		}
		return currentSlot;
	}

	std::string uploadFile(std::string nr, int storrelse, char* data) {
		if (next != nullptr)
		{
			std::string res = next->uploadFile(nr, storrelse, data);
			checkSlotReadiness();
			return res;
		}
		return "Error: No next config";
	}

	std::string uploadConfig(std::string configData)
	{
		TvPortSlot* old = nullptr;
	    switchToNextMutex.lock();
		if (next != nullptr)
		{
			if (current == nullptr)
			{
				current = next;
			} 
			else {
				old = next;
			}
			currentSlot = current->slotNumber;
			next = nullptr;
		}
		switchToNextMutex.unlock();
		if (old != nullptr) {
			delete old;
		}
		int slot = getNextSlotNumber();
		next = new TvPortSlot(slot);
		std::string res = next->uploadConfig(configData);
		checkSlotReadiness();
		return res;
	}
protected:
	TvPortSlot *readSlot(int slot)
	{
		TvPortSlot *portSlot = new TvPortSlot(slot);
		portSlot->readSlot();
		return portSlot;
	}

	int getNextSlotNumber()
	{
		int slot = current == nullptr ? 1 : current->slotNumber + 1;
		if (slot > TVPORT_MAXIMUM_SLOT_NUMBER)
		{
			slot = TVPORT_MINIMUM_SLOT_NUMBER;
		}
		return slot;
	}

	void checkSlotReadiness()
	{
		if (next != nullptr && next->isReady && !next->isCorrupted)
		{
			next->cleanUnnecessaryFiles(false);
			currentSlot = next->slotNumber;
			ParamUtils::writeParameterSlot(currentSlot);
			next->readyToSwitch = true;
		}
	}
};
extern TvPortSlots tvPortSlots;
#endif
