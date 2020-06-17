#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function.

    // Acquire the mutex (will be released when it goes out of scope)
    std::unique_lock<std::mutex> lock(_mutex);

    // Wait on the condition variable until the message queue is not empty
    _condition.wait(lock, [this] { return !_queue.empty(); });

    // Pop the message off of the queue
    T msg = std::move(_queue.front());

    // And delete it, because C++'s back/front peek instead of pop.
    _queue.pop_front();

    return msg;
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

    // Acquire the mutex (will be released when it goes out of scope)
    std::lock_guard<std::mutex> lock(_mutex);

    // Push the message into the queue and notify waiters
    _queue.emplace_back(std::move(msg));
    _condition.notify_one();
}

/* Implementation of class "TrafficLight" */

TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.
     while (true) {

        // Get the most recent message from the queue
        TrafficLightPhase phase = _messageQueue.receive();

        if (phase == TrafficLightPhase::green)
            return;

        // 1 ms delay to avoid aggressive polling
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles.

    // Create the random number generator
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<> dist(4000, 6000);

    // Set start and duration
    auto start = std::chrono::system_clock::now();
    auto interval = dist(rng);

    while (true) {

        auto now = std::chrono::system_clock::now(); 
        long delta = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        if (delta > interval) {

            // Switch the light state
            if (_currentPhase == red)
                _currentPhase = green;
            else
                _currentPhase = red;

            // Send the current phase to the message queue
            _messageQueue.send(std::move(_currentPhase));

            // Reset start and duration
            start = std::chrono::system_clock::now();
            interval = dist(rng);
        }

        // 1 ms delay to avoid aggressive polling
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

