#ifndef SCHEDULER_H_
#define SCHEDULER_H_

#include <avr/io.h>
#include <stdlib.h>

///Up to this many tasks can be run, in addition to the idle task
#define MAXTASKS	8
#include <stdint.h>

template <class T> class LinkedList{
	// Struct inside the class LinkedList
	// This is one node which is not needed by the caller. It is just
	// for internal work.
	struct Node {
		T x;
		Node *next;
	};
	Node* newNode()
	{
		Node* const p = (Node*)malloc(sizeof(Node));
		// handle p == 0
		return p;
	}

	void deleteNode(void * p) // or delete(void *, std::size_t)
	{
		free(p);
	}

	// public member
	public:
	// constructor
	LinkedList(){
		head = NULL; // set head to NULL
	}

	// destructor
	~LinkedList(){
		Node *next = head;

		while(next) {              // iterate over all elements
			Node *deleteMe = next;
			next = next->next;     // save pointer to the next element
			deleteNode(deleteMe);       // delete the current entry
		}
	}

	// This prepends a new value at the beginning of the list
	void push(T val){
		Node *n = newNode();   // create new Node
		n->x = val;             // set value
		n->next = NULL;
		//  If the list is empty, this is NULL, so the end of the list --> OK
		if (head == NULL){
			head = NULL;
			head = n;
			tail = n;
		}
		else{
			tail->next = n;
			tail = n;
		}
		
	}

	// returns the first element in the list and deletes the Node.
	// caution, no error-checking here!
	T pop(){
		Node *n = head;
		T ret = n->x;

		head = head->next;
		deleteNode(n);
		return ret;
	}
	
	T* front(){
		return &(head->x);
	}
	
	bool empty(){
		return head == NULL;
	}

	// private member
	private:
	Node *head; // this is the private member variable. It is just a pointer to the first Node
	Node *tail;
};

typedef struct {
	char port;
	uint8_t pin;
} arg_t;

///A task callback function
typedef void (*task_cb)(LinkedList<arg_t>&);


typedef struct { // one-shot tasks
	int32_t remaining_time;
	int32_t max_time;
	uint8_t is_running;
	task_cb callback;
	int priority;
	LinkedList<arg_t> args;
} oneshot_t;
//typedef void (*arg_t)();
//typedef void (*oneshot_t)();

/**
 * Initialise the scheduler.  This should be called once in the setup routine.
 */
void Scheduler_Init();

/**
 * Start a task.
 * The function "task" will be called roughly every "period" milliseconds starting after "delay" milliseconds.
 * The scheduler does not guarantee that the task will run as soon as it can.  Tasks are executed until completion.
 * If a task misses its scheduled execution time then it simply executes as soon as possible.  Don't pass stupid
 * values (e.g. negatives) to the parameters.
 *
 * \param id The tasks ID number.  This must be between 0 and MAXTASKS (it is used as an array index).
 * \param delay The task will start after this many milliseconds.
 * \param period The task will repeat every "period" milliseconds.
 * \param task The callback function that the scheduler is to call.
 */
void Scheduler_StartPeriodicTask(int16_t, int16_t, task_cb, arg_t arr[]);
void Schedule_OneshotTask(int32_t, int32_t, uint8_t, task_cb, int, arg_t arr[]);
uint32_t min(uint32_t, uint32_t);
/**
 * Go through the task list and run any tasks that need to be run.  The main function should simply be this
 * function called as often as possible, plus any low-priority code that you want to run sporadically.
 */
uint32_t Scheduler_Dispatch_Periodic();
uint32_t Scheduler_Dispatch_Oneshot(uint32_t);
uint32_t Scheduler_RunTask_Oneshot(oneshot_t);

#endif /* SCHEDULER_H_ */
