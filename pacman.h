
extern const unsigned short FIELD_WIDTH;
extern const unsigned short FIELD_HEIGHT; // DO NOT MAKE BIGGER! 
extern const unsigned short ADD_WIDTH; // for future additions of lives and such/ and for score
extern const unsigned short POINTS_PER_COOKIE;
extern const unsigned short INTRO_WIDTH;
extern const unsigned short INTRO_HEIGHT;
extern const unsigned short OVER_WIDTH;
extern const unsigned short OVER_HEIGHT;
extern const char* LEV_PATH;
extern const char* INTRO_PATH;
extern const char* OVER_PATH;
extern const float PAC_MOVE_TIME;
extern const float BOO_MOVE_TIME;
enum dirs{UP = 1, RT = 1u<<1, DN = 1u<<2, LF = 1u<<3}; // directions
enum field_obj{PASS = ' ', WALL = '#', COOKIE = '.', POWERUP = '*'}; // for loading the file
struct FCOORD{float X; float Y;};