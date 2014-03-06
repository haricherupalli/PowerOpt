#ifndef __MACROS_H__
#define __MACROS_H__

namespace POWEROPT {

#define DOSE_SENSITIVITY (-2.0) //nm/%
#define D 1.0 //original exposure dose
#define GRID_WIDTH 20
#define GRID_HEIGHT 20
#define CHANNEL_LENGTH 100 //nm
#define NUM_GATES 14
#define K 1 //top 50 critical paths
#define DELTA 2
#define SMOOTHNESS_CONSTRAINT
#define POWER_CONSTRAINT
#define W_SR 1
#define USE_SPEF
#define MIN_VAL 0.0001
//#define ANA_TIMING

//#define REPLACE_CELL_NAME

}

#endif //__MACROS_H__
