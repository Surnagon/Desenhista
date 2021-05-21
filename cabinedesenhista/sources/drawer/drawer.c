#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <errno.h>
#include <wiringPi.h>
#include "drawer/drawer.h"

// todo: receber comandos de outra thread (incluindo nome do arquivo)
// todo: criar enumeracao de comandos
// todo: criar codigos de retorno 

/** One second as nsecs */
#define DRAWER_1SEC_AS_NSEC (1000000000L)

/** Maximum distance between pixels in a continuous drawing */
#define DRAWER_MAX_CONTINUOUS_DISTANCE (3)

/** Time delay between readings of pixels, in which the task waits for commands  */
#define DRAWER_DELAY_NSEC (3000000L) 

/** String length in the file name template 'yy-mm-dd-hh-mm-ss.map' */
#define DRAWER_FILE_NAME_STRLEN (21)

/** PWM Pin of motor A */
#define DRAWER_MOTOR_A_PIN (1)

/** PWM Pin of motor B */
#define DRAWER_MOTOR_B_PIN (2)

/** PWM Pin of motor C */
#define DRAWER_MOTOR_C_PIN (3)

/**
 * @brief Structure to represent the cartesian position of the pixels
 */
typedef struct
{
	int x; /**< Horizontal position */
	int y; /**< Vertical position */
} drawer_cartesianpos_t;

/**
 * @brief Structure to represent the pixels
 */
typedef struct
{
   drawer_cartesianpos_t pos; /**< Pixel position */
   bool continuous; /**< Flag that infors if this pixel is connected with the previous pixel by a black line */
} drawer_pixel_t;

/**
 * @brief Structure of drawer context
 */
typedef struct
{
   FILE * fp; /**< File pointer to access the file with position of black pixels */
   bool * is_drawed; /**< Flag array to control which pixels on the file have been drawed */
   unsigned int nof_pixels; /**< Total number of pixels. */
   unsigned int pixel_count; /**< Number of pixels already drawed */
   drawer_pixel_t last_pixel; /**< Last drawed pixel */
   unsigned int last_pixel_id; /**< Number of the last pixel */
} drawer_context_t;

/**
 * @brief Enumeration of commands recieved by the drawer thread
 */
typedef enum
{
   DRAWER_COMMAND_NONE       = 0, /**< No command received by the drawer thread */
   DRAWER_COMMAND_STOP       = 1, /**< Command to stop the drawer routines */
   DRAWER_COMMAND_NEW        = 2, /**< Command to start a new drawing */
   DRAWER_COMMAND_ABORT      = 3, /**< Abort any drawing in progress */
   DRAWER_COMMAND_PIXEL_DONE = 4, /**< Last pixel drawing completed */
} drawer_command_t;

/**
 * @brief Enumeration of drawer states
 */
typedef enum
{
   DRAWER_STATE_IDLE = 0,  /**< Drawer is waiting to next file to draw. */
   DRAWER_STATE_BUSY = 1, /**< Last drawing request is in progress. */
   DRAWER_STATE_DONE = 2, /**< Waiting the drawer motion of last pixels */
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
/** Id of thread that controls the drawing motions */
static pthread_t drawer_motion_thread_id = 0;
/** Exit code of drawer thread. */
static int drawer_thread_ret = 0;
/** Exit code of drawer motion thread */
static int drawer_motion_thread_ret = 0;
/** Context of drawer command receiving */
static drawer_cmdcontext_t drawer_cmd = {.info = {.id = DRAWER_COMMAND_NONE}};
/** Flag that informs (when @b true) that the drawer routines are started */
static bool drawer_is_started = false;
/** Descriptor of message queue trough whith the pixels are transmitted to the task that controls the drawing motion*/
static mqd_t drawer_motion_mq;
/** Name of the message queue used to transmit the pixel positions to the thread that controls the drawing motions. */
static const char * drawer_motion_mq_name = "/mq_drawer_motion";
/** Mutext to control the reading access to the mqueue used to transmit positions to the drawing motion task */
static pthread_mutex_t drawer_motion_mq_mutext;


/**
 * @brief Function to send the request of a pixel drawing to the drawer motion task.
 *
 * @param[in] pixel is the information of requested pixel.
 * @param[out] queue_is_full receives @b true when the queue is full
 *
 * @return This function return 0 when the request is successfully performed,
 *         otherwise it returns the errno codes of function clock_gettime(),
 *         or mq_send().
 */
static int drawer_motion_request(const drawer_pixel_t * pixel, bool * queue_is_full)
{
   struct timespec timeout;
   int result;
   struct mq_attr attr;

   if (pixel == NULL)
   {
      return 0;
   }
   
   result = clock_gettime(CLOCK_REALTIME, &timeout);
   if (result)
   {
      printf("clock_gettime error %d \r\n", errno);
      return errno;
   }

   result = mq_getattr(drawer_motion_mq, &attr);
   if (result)
   {
      printf("Error reading mq attr, error code %d \r\n", errno);
      return errno;
   }

   result = mq_send(drawer_motion_mq, (const char *)pixel, sizeof(drawer_pixel_t),0);
   if (result)
   {
      if (errno = EAGAIN)
      {
         if (queue_is_full != NULL)
         {
            *queue_is_full = true;
         }
      }
      else
      {
         printf("Error sending pixel to drawer motion task, erro code %d \r\n", errno);
         return errno;
      }
   }

   if (queue_is_full != NULL)
   {
      *queue_is_full = false;
   }

   return 0;
}

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
      case DRAWER_COMMAND_PIXEL_DONE:
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
      printf("pthread_cond_signal error %d \r\n", result);
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
      printf("clock_gettime error %d \r\n", errno);
      return errno;
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
static unsigned int drawer_euclideandistance(const drawer_cartesianpos_t *a, const drawer_cartesianpos_t *b)
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
 * @brief Function to get the next pixel to be drawed.
 *
 * @param[in] ctx is the drawer context.
 * @param[in] reset forces to return to first pixel of the file.
 * @param[out] pixel receives the next pixel position.
 * @param[out] eof receives @b true if all pixels have been read from the file.
 *
 * @return This function returns 0 if the reading is successfully performed, otherwise 
 *         it returns -1;
 */
static int drawer_getpixel(drawer_context_t * ctx, bool reset, drawer_pixel_t * pixel, bool * eof)
{
	char * read;
   char pixelstr[17];
   int result;
   unsigned int i;
   long int pos;
   bool next_end;
   bool prev_end;
   int prev;
   int next;
   drawer_cartesianpos_t read_prev;
   drawer_cartesianpos_t read_next;
   unsigned int lowest_dist = 0xFFFFFFFF;
   unsigned int lowest_dist_pos;
   unsigned int dist;

   if (ctx == NULL)
   {
      printf("Null drawer context \r\n");
   }

   if (reset)
   {
      result = 0;

      // Go to the end of file
      result = fseek(ctx->fp, 0, SEEK_END);
      if (result)
      {
         printf("fseek error %d \r\n", result);
      }

      // Get position of end of file
      pos = ftell(ctx->fp);
      if (pos == -1)
      {
         printf("ftell error %d \r\n", errno);
      }

      // Compute the number of lines in the file
      ctx->nof_pixels = pos/18;
      
      // Allocate memory to the flag array to control which pixels
      // have been drawed
      if (ctx->is_drawed != NULL)
      {
         free(ctx->is_drawed);
      }
      ctx->is_drawed = (bool *)malloc(ctx->nof_pixels*sizeof(bool));
      memset(ctx->is_drawed, 0, ctx->nof_pixels*sizeof(bool));
      ctx->pixel_count = 0;
      printf("File with %d pixels \r\n", ctx->nof_pixels);
   }
 
   if (eof != NULL)
   {
      *eof = false;
   }

   if (ctx->pixel_count == ctx->nof_pixels)
   {
      if (eof != NULL)
      {
         *eof = true;
      }
      return 0;
   }

   if (ctx->pixel_count == 0)
   {
      lowest_dist_pos = 0; 
   }
   else
   {
      i = 1;
      lowest_dist = 0xFFFFFFFF;

      // Search for the pixel that is the closest to the last drawed pixel.
      do
      {
         // Get position index of previous and next pixels on the file
         prev = ctx->last_pixel_id - i;
         next = ctx->last_pixel_id + i;

         // Check if the pixels is beyond the file limits
         prev_end = (prev <= 0);
         next_end = (next >= ctx->nof_pixels);
        
         if (!prev_end)
         {
            // Go to line of previous pixels
            result = fseek(ctx->fp, prev*18, SEEK_SET); 
            if (result)
            {
               return -1;
            }
          
            // Read line with target pixel
            read = fgets(pixelstr, 17, ctx->fp);
            if (read == NULL)
            {
               return -1;
            }

            // Get coordinates from read line
            sscanf(pixelstr,"%d %d", &read_prev.x, &read_prev.y);
         
            if (!ctx->is_drawed[prev])
            {
               dist = drawer_euclideandistance(&ctx->last_pixel.pos, &read_prev);
               if (dist < lowest_dist)
               {
                  lowest_dist = dist;
                  lowest_dist_pos = prev;
               }
            }
         }

         if (!next_end)
         {
            // Go to line with target pixel
            result = fseek(ctx->fp, next*18, SEEK_SET); 
            if (result)
            {
               return -1;
            }
          
            // Read line with target pixel
            read = fgets(pixelstr, 17, ctx->fp);
            if (read == NULL)
            {
               return -1;
            }

            // Get coordinates from read line
            sscanf(pixelstr,"%d %d", &read_next.x, &read_next.y);
          
            if (!ctx->is_drawed[next])
            {
               dist = drawer_euclideandistance(&ctx->last_pixel.pos, &read_next);
               if (dist < lowest_dist)
               {
                  lowest_dist = dist;
                  lowest_dist_pos = next;
               }
            }
         }

         if ((prev_end || (lowest_dist < read_prev.y)) &&
             (next_end || (lowest_dist < read_next.y)))
         {
            break;
         }

         i++;
      }while(!prev_end || !next_end); 

      // Go to line with target pixel
      result = fseek(ctx->fp, lowest_dist_pos*18, SEEK_SET); 
      if (result)
      {
         return -1;
      }
    
      // Read line with target pixel
      read = fgets(pixelstr, 17, ctx->fp);
      if (read == NULL)
      {
         return -1;
      }

      // Get coordinates from read line
      sscanf(pixelstr,"%d %d", &pixel->pos.x, &pixel->pos.y);
      pixel->continuous = (ctx->pixel_count && 
            (drawer_euclideandistance(&pixel->pos, &ctx->last_pixel.pos) <  DRAWER_MAX_CONTINUOUS_DISTANCE));
      memcpy(&ctx->last_pixel, pixel, sizeof(drawer_pixel_t));

      ctx->last_pixel_id = lowest_dist_pos;
      ctx->is_drawed[lowest_dist_pos] = 1;
   }

   ctx->pixel_count++;

   return 0;
}


/**
 * @brief Function to run that motion routines
 * 
 * @param[in,out] arg is unused.
 */
static void *drawer_motion_task(void *arg)
{
   mqd_t mq;
   int result;
   struct timespec timeout;
   drawer_pixel_t pixel;
   bool reset;
   unsigned int us;

   us = 2000 + rand()*2000/RAND_MAX;

   mq = mq_open(drawer_motion_mq_name, O_RDONLY);
   if (mq == -1)
   {
      printf("Error opening the message queue to receive the pixels. Errno: %d \r\n", errno);
      drawer_motion_thread_ret = errno;
      pthread_exit(&drawer_motion_thread_ret);
   }

   while(1)
   {
      while(1)
      {
         result = clock_gettime(CLOCK_REALTIME, &timeout);
         if (result)
         {
            printf("clock_gettime error %d \r\n", errno);
            drawer_motion_thread_ret = errno;
            pthread_exit(&drawer_motion_thread_ret);
         }

         reset = false;
         timeout.tv_sec += 10;
         result = mq_timedreceive(mq, (char *)&pixel, sizeof(drawer_pixel_t), NULL, &timeout);
         if (result == -1)
         {
            if (errno == ETIMEDOUT)
            {
               reset = true;
               printf("Drawer motion task timeout.", errno);
            }
            else
            {
               printf("mq_timedreceive error %d \r\n", errno);
            }
         }
         else if (result == sizeof(pixel))
         {
            break;
         }
      }

      reset |= (pixel.pos.x < 0);

      if (pixel.pos.x == -2)
      {
         drawer_notify_pixel_done();
      }
      
      if (reset)
      {
         printf("Moving pen back to initial position \r\n");
      }
      else
      {
         printf("\t :%07d %07d %d \r\n", pixel.pos.x, pixel.pos.y, pixel.continuous);
      }

      usleep(us);

      //drawer_notify_pixel_done();
   }
}

/**
 * @brief Function to clear all messages from message queue
 */
static int drawer_clear_mq(void)
{
   int result;
   drawer_pixel_t pixel;
   unsigned int cnt;

   do
   {
      result = mq_receive(drawer_motion_mq, (char *)&pixel, sizeof(drawer_pixel_t),NULL);
   }while(result==0);

   if (errno != EAGAIN)
   {
      printf("\r\n Error clearing the message queue, mq_receive error %d \r\n", errno);
      return errno;
   }

   return 0;
}

/**
 * @brief Function that runs the drawer routines
 *
 * @param[in,out] arg is unused
 */
static void * drawer_task(void * arg)
{
   drawer_context_t ctx;
	drawer_pixel_t current;
   drawer_state_t state = DRAWER_STATE_IDLE;
   drawer_cmdinfo_t cmd;
   int result;
   bool queue_is_full = false;
   bool eof;

	while (1)	
	{
      // Timed wait for new commands
      result = drawer_cmdreceive(&cmd);
      if (result)
      {
         printf("Error waiting for command %d \r\n", result);
         if (ctx.fp != NULL)
         {
            fclose(ctx.fp);
         }

         drawer_thread_ret = result;
         pthread_exit(&drawer_thread_ret);
      }

		switch (cmd.id)
		{
			case DRAWER_COMMAND_STOP:
				if (ctx.fp != NULL)
				{
					fclose(ctx.fp);
				}
            drawer_clear_mq();
				drawer_thread_ret = 0;
				pthread_exit(&drawer_thread_ret);
			break;	

         case DRAWER_COMMAND_ABORT:
				if (ctx.fp != NULL)
				{
					fclose(ctx.fp);
               ctx.fp = NULL;
				}
            drawer_clear_mq();
            state = DRAWER_STATE_IDLE;
         break; 

			case DRAWER_COMMAND_NEW: 
            // Close opened file
				if (ctx.fp != NULL)
				{
					fclose(ctx.fp);
               ctx.fp = NULL;
				}
		  
            
            printf("Opening file %s \r\n", cmd.param.new_draw.file_name);

            // Open new file for reading
			   ctx.fp = fopen(cmd.param.new_draw.file_name,"r");
				if (ctx.fp == NULL)
				{
					printf("Error opening the file %s \r\n", cmd.param.new_draw.file_name);
               state = DRAWER_STATE_IDLE;
				}
            else
            {
               drawer_clear_mq();


               // Send a reserved coordinate that tells the motion task to 
               // move the pen to initial position.
               current.pos.x = -1;
               drawer_motion_request(&current, NULL);

               // Get the first pixel
               result = drawer_getpixel(&ctx, true, &current, &eof);
               if (result)
               {
                  fclose(ctx.fp);
                  ctx.fp = NULL;

                  state = DRAWER_STATE_IDLE;
               }
               else
               {
                  state = DRAWER_STATE_BUSY;
               }
            }

			break;

         case DRAWER_COMMAND_PIXEL_DONE:
            printf("\r\n Done o/");
            state = DRAWER_STATE_IDLE;
         break;

         case DRAWER_COMMAND_NONE:
			default:
         break;
		}

      switch (state)
      {
         case DRAWER_STATE_BUSY:
            // Request the drawing of current pixel
            result = drawer_motion_request(&current, &queue_is_full);
            if (result)
            {
               printf("Error drawing the pixel %d %d, error code %d \r\n", current.pos.x, current.pos.y, result);
               fclose(ctx.fp);
               ctx.fp = NULL;

               state = DRAWER_STATE_IDLE;
               break;
            }

            if (!queue_is_full)
            {
               if (!eof)
               {
                  // While the current pixel is being drawed, search the next pixel
                  result = drawer_getpixel(&ctx, false, &current, &eof);
                  if (result)
                  {
                     printf("Error reading next pixel, error %d\r\n", result);
                     fclose(ctx.fp);
                     ctx.fp = NULL;

                     state = DRAWER_STATE_IDLE;
                  }

                  if (eof)
                  {
                     printf(" Last pixel read from file \r\n");
                     current.pos.x = -2;
                  }
               }
               else
               {
                  printf("\r\n All pixeis queued for drawing \r\n");
                  state = DRAWER_STATE_DONE;
               }
            }
         break;

         case DRAWER_STATE_DONE:
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
   static struct mq_attr mq_attr;

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
      return result;
   }

   result = pthread_cond_init(&drawer_cmd.cond, &attr);
   if (result)
   {
      printf("pthread_cond_init error %d \r\n", result);
      return result;
   }

   result = pthread_mutex_init(&drawer_cmd.mutex, NULL);
   if (result)
   {
      printf("pthread_mutex_init error %d \r\n", result);
      return result;
   }

   result = pthread_mutex_init(&drawer_motion_mq_mutext, NULL);
   if (result)
   {
      printf("pthread_mutex_init error %d \r\n", result);
      return result;
   }

   mq_unlink(drawer_motion_mq_name);
   mq_attr.mq_flags = 0;
   mq_attr.mq_maxmsg = 10;
   mq_attr.mq_msgsize = sizeof(drawer_pixel_t);
   mq_attr.mq_curmsgs = 0;
   // Open message queue
   // Flags:
   //    O_CREATE: Create the message queue if it does not exist
   //    O_WRONLY: This descriptor is used only to write into the message queue
   // Mode:
   //    S_IRUSR: User has read permission
   //    S_IWUSR: User has write permission
   //    S_IRGRP: Group has read permission
   //    S_IWGRP: Group has write permission
   drawer_motion_mq = mq_open(drawer_motion_mq_name,
         (O_CREAT | O_RDWR | O_NONBLOCK), (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP), &mq_attr);
   if (drawer_motion_mq == -1)
   {
      printf("mq_open error creating the message queue %s, error: %d \r\n", drawer_motion_mq_name, errno);
      return errno;
   }

   if (wiringPiSetup() == -1)
   {
      printf("Error initializing the WiringPi lib \r\n");
      return -1;
   }

   pinMode(DRAWER_MOTOR_A_PIN, PWM_OUTPUT);
   pinMode(DRAWER_MOTOR_B_PIN, PWM_OUTPUT);
   pinMode(DRAWER_MOTOR_C_PIN, PWM_OUTPUT);

   pwmSetMode(PWM_MODE_MS);
   pwmSetClock(3840);
   pwmSetRange(100);


	// Create the drawer thread
	result = pthread_create(&drawer_thread_id, NULL, &drawer_task, NULL);
	if (result != 0)
	{
		printf("pthread_create error %d. Error creating drawer task \r\n", result);
      return result;
	}
   
	// Create the drawer thread
	result = pthread_create(&drawer_motion_thread_id, NULL, &drawer_motion_task, NULL);
	if (result != 0)
	{
		printf("pthread_create error %d. Error creating drawer motion task \r\n", result);
		return result;
   }
   drawer_is_started = true;

	return 0;
}

/** 
 * @brief Function to notify the completion of a pixel drawing
 *
 * @retunr This function returns 0 when it is successfully performed,
 *         otherwise it returns one of the codes returned by the
 *         function #drawer_sendcmd();
 */
int drawer_notify_pixel_done(void)
{
   int result;
   void * thread_ret;

   result = drawer_sendcmd(DRAWER_COMMAND_PIXEL_DONE);
   if (result)
   {
      printf("Error sending pixel-done command %d \r\n", result);
      return result;
   }

   return result;
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

   pthread_mutex_lock(&drawer_cmd.mutex);

   drawer_cmd.info.id = DRAWER_COMMAND_NEW;
   strncpy(drawer_cmd.info.param.new_draw.file_name
         , file_name, DRAWER_FILE_NAME_STRLEN);

   result = pthread_cond_signal(&drawer_cmd.cond);

   pthread_mutex_unlock(&drawer_cmd.mutex);
   
   if (result)
   {
      printf("pthread_cond_signal error %d \r\n", result);
   }

   return result;
}
