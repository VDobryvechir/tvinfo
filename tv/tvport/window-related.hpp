#ifndef WINDOW_RELATED_HPP
#define WINDOW_RELATED_HPP
class WindowRelatedUtils {
public:
	static void getDesktopResolution(int& horizontal, int& vertical);
	static void setFullScreenMode(std::string winName);
	static void windowCleaning();
};

#endif