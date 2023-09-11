# Real-Time Operating System (RTOS) Communication Project

## Overview

This project demonstrates the use of FreeRTOS on an embedded target emulation board within Eclipse CDT. It involves the coordination of four tasks through a fixed-size queue. The tasks consist of three sender tasks, each with its own characteristics, and a receiver task. The primary goal is to showcase task synchronization and communication in a real-time environment.

## Task Descriptions

### Sender Tasks

1. **Sender 1 and Sender 2**: These two sender tasks have equal priorities. Each task periodically sends a message to the shared queue. The message contains the string "Time is XYZ," where XYZ represents the current system time in ticks. If the queue is full when a sender attempts to send a message, it results in a failed sending operation, and a counter tracking blocked messages is incremented. On successful message transmission, a counter for total transmitted messages is incremented. Both sender tasks then enter a sleep state for a random period.

2. **Sender 3**: This sender task has a higher priority compared to Sender 1 and Sender 2. It follows the same logic for sending messages, blocking on queue full, and updating message counters. After sending a message, Sender 3 also enters a random sleep state.

### Receiver Task

The receiver task periodically wakes up at fixed intervals. It checks the shared queue for any incoming messages. If a message is present, it reads and processes it, incrementing a counter for received messages. If the queue is empty, the receiver task goes back to sleep immediately. Importantly, the receiver reads one message at a time, even if multiple messages are available in the queue.

### Task Scheduling

The sleep and wake behavior of all three sender tasks is managed using individual timers. Each sender task has its dedicated timer, which, upon expiration, releases a semaphore. This semaphore unblocks the respective sender task, allowing it to send messages to the queue. Similarly, the receiver task has its timer that unblocks it when it's time to check the queue for incoming messages.

## Completion Criteria

When the receiver task has successfully received 1000 messages, the following actions are taken:

1. Print the total number of successfully sent messages and the total number of blocked messages.
2. Display statistics for each sender task, including the high-priority sender and the two lower-priority senders.
3. Reset the counters for successfully sent messages, blocked messages, and received messages.
4. Clear the shared message queue.
5. Configure the sender timers' period (Tsender) to new values drawn from predefined arrays representing lower and upper bounds for a uniform distribution.
6. If all values in the timer period arrays are exhausted, the system terminates with a message "Game Over."

## Initial Configuration

In the initial system startup, Tsender values are set to 50 and 150 msec, and Treceiver remains fixed at 100 msec.

**Note:** This project provides a practical demonstration of real-time task synchronization and communication, making it a valuable learning resource for embedded systems development with FreeRTOS.
