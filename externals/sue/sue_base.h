#ifndef SUE_BASE_H_SENTRY
#define SUE_BASE_H_SENTRY

/*! \file sue_base.h
    \brief The SUE library plain C base
   
    This file implements the main plain C 'classes' of the library: 
     - sue_event_selector
     - sue_fd_handler
     - sue_timeout_handler
     - sue_signal_handler
     - sue_loop_hook
    These are likely needed in almost any event-driven application.
 */

//! Event selector
/*! This 'class' provides an object-oriented framework for the unix pselect(2)
    system call. It uses objects of:
     - sue_fd_handler for file descriptor oriented events, 
     - sue_timeout_handler class for time events,
     - sue_signal_handler class for signal events,
     - sue_loop_hook to handle various postponed tasks (called
            on every iteration of the main loop)
    The 'class' consists of the sue_event_selector structure and the
    methods named sue_event_selector_XXX.

    You need to create an object of this class (that is, to create a
    variable of the struct sue_event_selector type, and then call the
    sue_event_selector_init() method on it), then decide what events you 
    wish to handle from the very start, create the appropriate objects 
    for these events, register them using the appropriate methods, then
    call the sue_event_selector_go() and enjoy. Certainly you can register
    more events and unregister some of the registered events at any time you
    want, but please note if you define no events before calling the 'go'
    method then it will loop forever.  Remember, you're creating an
    event-driven application!

    \note This class is THE good point to start with. You *ALWAYS* need 
      one (and in almost all cases only one) object of this class. 
      The 'go' method of this class is the main loop of your application,
      and the sue_event_selector_break() is used to break the main loop.
 */

struct sue_event_selector {
         //! Sorted list of timeout handlers
    struct sue_event_selector_timeout_item *timeouts;
         //! Signal wanters list
    struct sue_event_selector_signal_item *signalhandlers;
         //! File descriptor handlers 
         /*! Array indexed by descriptor values themselves.
             That is, fdhandlers[3] is a pointer to the FdHandler
             for fd=3, if any, or NULL. Array is resized as necessary.
          */
    struct sue_fd_handler** fdhandlers;
         //! Current size of the fdhandlers array
    int fdhandlerssize;
         //! Max. value of currently used descriptors
    int max_fd;
         //! List of loop hooks
    struct sue_event_selector_loophook_item *loophooks;
         //! Is it time to break the main loop?
         /*! This flag is cleared by 'go' method and may be set by
             'break' method. The main loop checks the flag after all the 
             handlers are called, and if it is set, the main loop quits.
          */
    int breakflag;
};

       //! Constructor
void sue_event_selector_init(struct sue_event_selector *s);

       //! Destructor
void sue_event_selector_done(struct sue_event_selector *s);

struct sue_fd_handler;
struct sue_timeout_handler;
struct sue_signal_handler;
struct sue_loop_hook;

       //! Register file descriptor handler
       /*! Registers an FD to be watched.
           \note The FD handler remains registered unless it is explicitly 
           unregistered with remove_fd_handler method.
           \warning It is assumed FD doesn't change when the handler is 
           registered.
           \warning If another handler for the same FD is registered,
           it is silently replaced.
        */
void sue_event_selector_register_fd_handler(struct sue_event_selector *s,
                                            struct sue_fd_handler *h);

       //! Removes the specified handler
       /*! Removes (unregisters) an FD handler. In case the handler is
           not registered, silently ignores the call
        */
void sue_event_selector_remove_fd_handler(struct sue_event_selector *s,
                                          struct sue_fd_handler *h);
       
       //! Register a timeout handler
       /*! Registers a time moment at which to wake up and call the 
           notification method. Time value (in seconds and microseconds)
           is specified by a sue_fd_handler subclass object which also 
           provides a callback function for notification.
           \note The timeout handler remains registered unless 
           either the moment comes and notification function is called
           OR it is explicitly unregistered with the remove method.
           \warning It is assumed that the value of the timeout doesn't
           change when it is registered.  Changind it will lead to 
           unpredictable behaviour. If you need to change the timeout, 
           then first unregister it, then change and register again.
        */
void sue_event_selector_register_timeout_handler(struct sue_event_selector *s,
                                               struct sue_timeout_handler *h);

       //! Removes the specified handler
       /*! Removes (unregisters) a timeout handler. In case the handler is
           not registered, silently ignores the call
        */
void sue_event_selector_remove_timeout_handler(struct sue_event_selector *s,
                                               struct sue_timeout_handler *h);

       //! Register a signal handler
       /*! Registers a signal specified with SUESignalHandler object  
        */
void sue_event_selector_register_signal_handler(struct sue_event_selector *s,
                                                struct sue_signal_handler *h);
       //! Remove a signal handler
void sue_event_selector_remove_signal_handler(struct sue_event_selector *s,
                                              struct sue_signal_handler *h);
	
       //! Register a loop hook
void sue_event_selector_register_loop_hook(struct sue_event_selector *s,
                                           struct sue_loop_hook *h);
       //! Remove a loop hook
void sue_event_selector_remove_loop_hook(struct sue_event_selector *s,
                                         struct sue_loop_hook *h);

	
       //! Main loop
       /*! This function 
            - sets up the fd_set's for read, write and except notifications
              in accordance to the set of registered file handlers
            - chooses the closest time event from the set of registered
              timeout handlers
            - sets the signal handling functions as appropriate (XXX ?!)
            - calls pselect(2)
            - checks if any handled signals happened
            - examines the fd_set's for FDs changed state and calls the
              appropriate callback methods
            - examines the timeouts list, selects those are now in 
              the past, removes each of them from the list of registered 
              objects and calls its notification method
            - checks for the loop breaking flag. If it is set, breaks the 
              loop and returns. Otherwise, continues from the beginning.
           \par
           The function is intended to be the main loop of your process.   
           \returns 0 in case it was correctly broken by
              sue_event_selector_break(), otherwise (in case pselect caused
              an error) returns -1.
        */
int sue_event_selector_go(struct sue_event_selector *s);

       //! Cause main loop to break (the 'go' method to return)
       /*! This function sets the loop breaking flag which causes
           the main loop to return. The flag is cleared each time 
           sue_event_selector_go() is called.  The flag is checked 
           after the main sequence of actions is performed, before
           repeating it
        */
void sue_event_selector_break(struct sue_event_selector *s);




//! File descriptor handler for sue_event_selector
/*!
  When we've got a file descriptor to be handled with pselect()
  (that is, with sue_event_selector), an instance of this structure is to
  be created.  What events to wait for on this descriptor is determined by
  want_read, want_write and want_except flags; once an event is detected,
  the function pointed to by fd_handle_event is called.
  The userdata pointer can be used in whatever manner the user wants,
  e.g. to point to the user's own object that represents the session.

  \warning It is assumed that the descriptor doesn't change after the object
  is registered. If you need to change it, then unregister the object, 
  change the descriptor and then register the object again.
  \note The callback function (the one pointed to by fd_handle_event) is
  free and welcome to change the want_XXX fields to reflect the needs of
  the user.
*/
struct sue_fd_handler {
    int fd;
    unsigned want_read:1, want_write:1, want_except:1;
    void *userdata;

        //! The callback function
        /*! The first parameter points to the sue_fd_hanler object
            for which the function is called; the resting three
            params are set to 0 or 1 (false/true) to reflect whether
            the descriptor detected to become ready to read, ready to
            write or whether an exception event happened
         */
    void (*handle_fd_event)(struct sue_fd_handler*, int, int, int);
};


//! Timeout handler for sue_event_selector
/*!
  Whenever we need to leave pelect() on (or after) some specific moments
  in time, every such moment is to be specified with an instance of
  this structure.

  \note The object remains registered until the timeout happens OR
  you unregister it explicitly.  Once the timeout is handled, the user
  is free to set the timeout to a new value and reuse the existing
  object.
  \warning Changing the time fields when the object is registered leads
  to an unpredictable behaviour.
  \warning From within the handle_timeout function, you should never try
  to remove other timeout_handler objects.  You can, however, remove any
  handlers of other types if you need, and add anything as well.
*/
struct sue_timeout_handler {
       //! seconds since epoch when the timeout is to happen
    long sec;  
       //! microseconds (in addition to sec)
    long usec;

    void *userdata;

    void (*handle_timeout)(struct sue_timeout_handler*);
};


      //! Set the timeout relative to the current time
      /*! Set the time interval after which the timeout is to happen.
          The method actually calls gettimeofday(3) and adds the arguments
          to the values returned.
          \param a_sec - seconds
          \param a_usec - microseconds
       */
void sue_timeout_handler_set_from_now(struct sue_timeout_handler *th,
                                      long a_sec, long a_usec);



//! Signal handler for sue_event_selector
/*! When we wish the sue_event_selector to catch some signals and notify us
    we create a sue_signal_handler object. There can be several objects
    for the same signal, in which case callback functions will be called for
    all the objects when the signal comes. 

    \note It is the Selector's duty to establish the appropriate signal
    handlers.  The existing signal disposition is saved when the handler
    is installed; if you unregister the last sue_signal_handler for a
    given signal, the saved disposition is then restored.

    \note All the signals handled by the library are blocked outside of
    pselect() and are only enabled for the time your application is blocked
    on the pselect() call.  In an event-based application, you souldn't be
    using blocking calls anyway, but if you do, please note your
    application will not respond to signals during such blockings.
 */
struct sue_signal_handler {
    int signo;

    void *userdata;

    void (*handle_signal)(struct sue_signal_handler*, int cnt);
};


//! Loop hook for sue_event_selector
/*! This object is to be used in case we have (or can have) something
    to be done at the end of each main loop iteration, such as to perform
    tasks postponed from various event handlers.

    \warning It is okay for the hook to de-register itself in the selector
             object from within the loop_hook function, and it can
             register/remove handlers of other types, but it must NEVER
             try to de-register other sue_loop_hook objects.
 */
struct sue_loop_hook {
    void *userdata;
    void (*loop_hook)(struct sue_loop_hook*);
};


#ifndef SUE_DEBUG
#define SUE_DEBUG 1
#endif

#if SUE_DEBUG
#include <assert.h>
#define SUE_ASSERT(x) assert((x))
   /* actually, this implementation is to change one day;
      definitely we shouldn't force the 'standard' assert in */
#else
#define SUE_ASSERT(x) do{}while(0)
#endif

#endif
