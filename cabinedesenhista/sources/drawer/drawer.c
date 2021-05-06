#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <string.h>

#include "drawer/drawer.h"

// todo: receber comandos de outra thread (incluindo nome do arquivo)
// todo: criar enumeracao de comandos
// todo: criar codigos de retorno 

/** One second as nsecs */
#define DRAWER_1SEC_AS_NSEC (1000000000L)

/** Maximum distance between pixels in a continous drawing */
#define DRAWER_MAX_CONTINUOUS_DISTANCE (3)


/** Time delay between drawing of consecutive pixels */
#define DRAWER_DELAY_NSEC (200000000L) 


/** String length in the file name template 'yy-mm-dd-hh-mm-ss.map' */
#define DRAWER_FILE_NAME_STRLEN (21)

/**
 * @brief Structure to represent the cartesian position of the pixels
 */
typedef struct
{
	int x; /**< Horizontal position */
	int y; /**< Vertical position */
} drawer_cartesianpos_t;

/**
 * @brief Enumeration of commands recieved by the drawer thread
 */
typedef enum
{
   DRAWER_COMMAND_NONE  = 0, /**< No command received by the drawer thread */
   DRAWER_COMMAND_STOP  = 1, /**< Command to stop the drawer routines */
   DRAWER_COMMAND_NEW   = 2, /**< Command to start a new drawing */
   DRAWER_COMMAND_ABORT = 3, /**< Abort any drawing in progress */
} drawer_command_t;

/**
 * @brief Enumeration of drawer states
 */
typedef enum
{
   DRAWER_STATE_IDLE = 0,  /**< Drawer is waiting to next file to draw. */
   DRAWER_STATE_BUSY = 1, /**< Last drawing request is in progress. */
} drawer_state_t;

/**
 * @brief Information of commands received by drawer thread
 */
typedef struct
{
   drawer_command_t id; /**< Command id */

   union
   {
      /** Information of the new file command */
      struct
      {
         char file_name[DRAWER_FILE_NAME_STRLEN]; /**< Name of the file with black pixels coordinates */
      } new_draw;
   } param;
} drawer_cmdinfo_t;



/**
 * @brief Context of commands received by the drawer thread 
 */
typedef struct
{
   drawer_cmdinfo_t info; /**< Commandi information */
   pthread_cond_t cond; /**< Conditional variable used to receive the commands notification */
   pthread_mutex_t mutex; /**< Mutex used to control the access to conditional variable and command info. */
} drawer_cmdcontext_t;

/** Id of thread that runs the drawer routines */
static pthread_t drawer_thread_id = 0;
/** Exit code of drawer thread. */
static int drawer_thread_ret = 0;
/** Context of drawer command receiving */
static drawer_cmdcontext_t drawer_cmd = {.info = {.id = DRAWER_COMMAND_NONE}};
/** Flag that informs (when @b true) that the drawer routines are started */
static bool drawer_is_started = false;

/**
 * @brief Function to change command id and notify the change
 *
 * @param[in] id is the new command id
 *
 * @return This function returns 0 when it is successfully performed,
 *         othewise it returns one of the codes returned by the
 *         function pthread_cond_signal().
 */
static int drawer_sendcmd(drawer_command_t id)
{
   int result;

   switch(id)
   {
      case DRAWER_COMMAND_ABORT:
      case DRAWER_COMMAND_STOP:
      break;
      
      case DRAWER_COMMAND_NEW:
      case DRAWER_COMMAND_NONE:
      default:
         return -1;
      break;
   }
   
   pthread_mutex_lock(&drawer_cmd.mutex);

   drawer_cmd.info.id = id;

   result = pthread_cond_signal(&drawer_cmd.cond);

   pthread_mutex_unlock(&drawer_cmd.mutex);
   
   if (result)
   {
      printf("pthread_cond_signal error %d", result);
   }

   return result;
}

/**
 * @brief Function to wait for commands
 *
 * @param[in] cmd receives the command information.
 *
 * @return This function returns 0 when it is succesffully performed,
 *          otherwise it returns the error code returned by:
 *          clock_gettime().
 */
static int drawer_cmdreceive(drawer_cmdinfo_t * cmd)
{
   int result;
   struct timespec timeout;
   
   pthread_mutex_lock(&drawer_cmd.mutex);
   
   result = clock_gettime(CLOCK_MONOTONIC, &timeout);
   if (result)
   {
      printf("clock_gettime error %d \r\n", result);
      return result;
   }

   // Sanitize increment of current clock time to get absolute timeout
   timeout.tv_nsec += DRAWER_DELAY_NSEC;
   if (timeout.tv_nsec > DRAWER_1SEC_AS_NSEC)
   {
         timeout.tv_sec++;
         timeout.tv_nsec -= DRAWER_1SEC_AS_NSEC;
   }

   while(drawer_cmd.info.id == DRAWER_COMMAND_NONE)
   { 
      result = pthread_cond_timedwait(&drawer_cmd.cond, &drawer_cmd.mutex, &timeout);
      if (result)
      {
         if (result == ETIMEDOUT)
         {
            break;
         }
         else
         {
            printf("pthread_cond_timedwait error %d \r\n", result);
         }
      }
   }
   
   
   memcpy(cmd, &drawer_cmd.info, sizeof(drawer_cmdinfo_t));
   drawer_cmd.info.id = DRAWER_COMMAND_NONE;

   pthread_mutex_unlock(&drawer_cmd.mutex);

   return 0;
}

/**
 * @brief Function to compute the euclidean distance between two cartesian coordinates.
 *
 * @param[in] a is the pointer to the cartesian position of first pixel
 * @param[in] b is the pointer to the cartesian position of second pixel
 *
 * @return This function returns the euclidean distance between input positions.
 *         If any input :parameter is NULL, this function returns 0.
 */
static unsigned int drawer_euclideandistance(const drawer_cartesianpos_t * a, const drawer_cartesianpos_t * b)
{
	unsigned int distance;
	drawer_cartesianpos_t dif;

	if ((a == NULL) || (b == NULL))
	{
		return 0;
	}

	dif.x = (a->x - b->x);
	dif.y = (a->y - b->y);

	distance = sqrt(dif.x*dif.x + dif.y*dif.y);

	return distance;
}


/**
 * @brief Function to paint a pixel
 *
 * @param[in] pixel is the cartesian position of the pixel
 * @param[in] continuous informs if the pixel is 'connected' to previous painted pixel.
 */
static void drawer_paint(const drawer_cartesianpos_t * pixel, bool continuous)
{
	if (!continuous)
	{
		// Pen up
	}

	// Pen move

	if (!continuous)
	{
		// Pen down
	}
}

/**
 * @brief Function that run tha drawer routines
 *
 * @param[in,out] arg is unused
 */
static void * drawer_task(void * arg)
{
	FILE * fp = NULL;
	char * line = NULL;
	ssize_t read;
	size_t len;
	unsigned int pixel_count;
	unsigned int distance;
	drawer_cartesianpos_t current;
	drawer_cartesianpos_t last;
	bool continuous = false;
   drawer_state_t state = DRAWER_STATE_IDLE;
   drawer_cmdinfo_t cmd;
   int result;

	while (1)	
	{
      // Timed wait for new commands
      result = drawer_cmdreceive(&cmd);
      if (result)
      {
         printf("Error waiting for command %d \r\n", result);
         if (fp != NULL)
         {
            fclose(fp);
         }

         if (line != NULL)
         {
            free(line);
         }

         drawer_thread_ret = result;
         pthread_exit(&drawer_thread_ret);
      }

		switch (cmd.id)
		{
			case DRAWER_COMMAND_STOP:
				if (fp != NULL)
				{
					fclose(fp);
				}

            if (line != NULL)
            {
               free(line);
            }

				drawer_thread_ret = 0;
				pthread_exit(&drawer_thread_ret);
			break;	

         case DRAWER_COMMAND_ABORT:
				if (fp != NULL)
				{
					fclose(fp);
               fp = NULL;
				}

            if (line != NULL)
            {
               free(line);
               line = NULL;
            }

            state = DRAWER_STATE_IDLE;
         break; 

			case DRAWER_COMMAND_NEW: 

            // Close opened file
				if (fp != NULL)
				{
					fclose(fp);
               fp = NULL;
				}

            if (line != NULL)
            {
               free(line);
               line = NULL;
            }
		  
            printf("Opening file %s \r\n", cmd.param.new_draw.file_name);

            // Open new file for reading
			   fp = fopen(cmd.param.new_draw.file_name,"r");
				if (fp == NULL)
				{
					printf("Error opening the file %s \r\n", cmd.param.new_draw.file_name);
               state = DRAWER_STATE_IDLE;
				}
            else
            {
               state = DRAWER_STATE_BUSY;
   				pixel_count = 0;
            }
			break;

			default:
			break;
		}


      switch (state)
      {
         case DRAWER_STATE_BUSY:
            // Get next line (DYNAMIC ALLOCATION) 
            read = getline(&line, &len, fp);
            if (read == -1)
            {
               fclose(fp);
               fp = NULL;

               if (line != NULL)
               {
                  free(line);
                  line = NULL;
               }

               state = DRAWER_STATE_IDLE;
            }
            else
            {
               // Get coordinates from line
               sscanf(line,"%d %d", &current.x, &current.y);
               printf("\t: %d %d\r\n", current.x, current.y);
               continuous = (pixel_count && (drawer_euclideandistance(&current, &last) <  DRAWER_MAX_CONTINUOUS_DISTANCE));

               drawer_paint(&current, continuous);
            }

         break;

         default:
            state = DRAWER_STATE_IDLE;
         break;  
      }
	}
}

/** 
 * @brief Function to start the drawer routines
 */
int drawer_start(void)
{
	int result;
	int join_result;
   pthread_condattr_t attr;

   if (drawer_is_started)
   {
      return 0;
   }

   // Set conditional variable to use the monotonic clock,
   // which is not affected by changes in system time, 
   // such that the interval between pixels paiting is ensured.
   result =  pthread_condattr_init(&attr);
   if (result)
   {
      printf("pthread_condattr_init error %d \r\n", result);
      return result;
   }

   result = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
   if (result)
   {
      printf("pthread_condattr_setclock error %d \r\n", result);
   }

   result = pthread_cond_init(&drawer_cmd.cond, &attr);
   if (result)
   {
      printf("pthread_cond_init error %d \r\n", result);
   }

   result = pthread_mutex_init(&drawer_cmd.mutex, NULL);
   if (result)
   {
      printf("\r\n pthread_mutex_init error %d \r\n", result);
   }

	// Create the drawer thread
	result = pthread_create(&drawer_thread_id, NULL, &drawer_task, NULL);
	if (result != 0)
	{
		return result;
	}
   
   drawer_is_started = true;

	return 0;
}

/** 
 * @brief Function to stop the drawer task
 *
 * @retunr This function returns 0 when it is successfully performed,
 *         otherwise it returns one of the codes returned by the
 *         function #drawer_sendcmd();
 */
int drawer_stop(void)
{
   int result;
   void * thread_ret;

   if (!drawer_is_started)
   {
      return 0;
   }
   
   printf("Sending stop command \r\n");

   result = drawer_sendcmd(DRAWER_COMMAND_STOP);
   if (result)
   {
      printf("Error sending stop command %d \r\n", result);
      return result;
   }

   result = pthread_join(drawer_thread_id, &thread_ret);
   if (result)
   {
      printf("pthread_join error %d \r\n", result);
      return result;
   }
      
   printf("Drawer routines stopped, exit code: %d \r\n", *(int *)thread_ret);
   

   return result;
}

/**
 * @brief Function to abort any drawing in progress
 *
 * @retunr This function returns 0 when it is successfully performed,
 *         otherwise it returns one of the codes returned by the
 *         function #drawer_sendcmd();
 */
int drawer_abort(void)
{
   int result;

   printf("Sending abort command \r\n");

   result = drawer_sendcmd(DRAWER_COMMAND_ABORT);

   return result;
}

/**
 * @brief Function to request new draw 
 *
 * @param[in] file_name is the name of the file with black pixels coordinates.
 *
 * @retunr This function returns 0 when it is successfully performed,
 *         otherwise it returns one of the codes returned by the
 *         function #drawer_sendcmd();
 */
int drawer_request(const char * file_name)
{
   int result = 0;

   if (file_name == NULL)
   {
      return -1;
   }

   if (!drawer_is_started)
   {
      printf("Error: drawer routines are not started \r\n");
      return -1;
   }

   printf("request file name %s \r\n", file_name);

   pthread_mutex_lock(&drawer_cmd.mutex);

   drawer_cmd.info.id = DRAWER_COMMAND_NEW;
   strncpy(drawer_cmd.info.param.new_draw.file_name
         , file_name, DRAWER_FILE_NAME_STRLEN);

   result = pthread_cond_signal(&drawer_cmd.cond);

   pthread_mutex_unlock(&drawer_cmd.mutex);
   
   if (result)
   {
      printf("pthread_cond_signal error %d", result);
   }

   return result;
}
