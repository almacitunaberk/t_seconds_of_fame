# t_seconds_of_fame

This is a project demonstrating how to coordinate many threads using mutex locks and pthread_condition variables.
The contributors are Tunaberk Almaci and Bümin Aybars İnci.

## Implementation

- We created is_this_question_asked boolean array, and question_asked condition variable array together with question_asked_mutex mutex array to guard the boolean variable array. The value of the boolean indicates for that index indicates that the moderator asked a new question.
- We created bool has_all_commentators_decided_array boolean array, and all_commentators_decided_array condition variable array together with all_commentators_decided_mutex_array mutex array to guard the boolean variable. The value stored in that index indicates that the commentators decided to answer or not for the current question in that index. 
- We created may_i_speak boolean to store whether the moderator gave permission to that commentator to speak. We guard this variable with permission_to_speak condition array and permission_to_speak_mutex mutex array.
- We created finished_speaking boolean array to store whether the commentator at a particular index has finished speaking. We guard this variable with done_speaking condition array and done_speaking_mutex mutex array.
- We created is_questions_over boolean to indicate whether the moderator asked all the questions. We guard this variable with is_program_over condition variable and is_program_over_mutex mutex variable.
- We created number_of_decided_commentators to store the number of commentators who decided to answer or not. Since this variable is shared across commentator threads, we guard this variable with deciding_process_mutex mutex.
- We used number_of_wanting_to_answer_commentators variable to store the number of commentators who decided to answer to the current question. Since this variable is shared across commentator threads, we guard this variable with wanting_to_answer_mutex mutex.
- We used answering_queue to store the commentators wanting to answer to the current question in a FIFO manner. Since commentators share this variable among each other, we guard it with answering_queue_mutex.
- We used is_breaking_news_event_happening boolean variable to indicate whether there is a breaking news event happening currently. Since this value is shared across commentators, moderator, breaking_news_event_listener as well as the main thread, we guard this variable with breaking_news_event_happening condition variable and breaking_news_event_happening_mutex.
- We used is_breaking_news_event_ended boolean varible to indicate that the previous breaking news event has ended. Since this value is also shared across all threads, we used breaking_news_event_ended condition and breaking_news_event_ended_mutex mutex to guard the value.


## Description of Logics in Threads:


### Main Thread:
- Parses the input taken from the user
- Initializes the variables according to the given input
- Initializes the variables we described above
- Initializes the moderator, commentators, breaking_news_event_listener threads
- Inside a while loop, every second, it checks if a breaking news event is present. If not, it creates one if the probability it calculates is enough
- If it decides that there is a breaking news event, it broadcasts this situation, sleeps for 5 seconds. Then, it broadcasts that the breaking news event has ended.
- If the questions are over, the main thread waits for other threads to finish


### Moderator:
- Checks whether there is currently a breaking news event
- If there isn't any, it asks the new question and informs the commentators that the new question has been asked
- It waits for commentators to decide to answer or not
- After all commentators decide, it goes through the answering queue to give permission to the next commentator that is in the queue
- If there isn't any breaking news event happening, it gives permission to the commentator by signaling that the commentator with a specific ID can speak
- It waits for the commentator to finish speaking
- After the commentator speaks, it gives permission to the next one from the queue. It goes like this until the queue is empty
- If the queue is empty, it moves to the next question


## Commentator:
- Each commenator waits for the next question to be asked
- If it is asked, each one calculates whether to answer or not
- If they decide to answer, they put themselves in the answeing queue
- If the number of decided commentators reaches the number of total commentators, it signals the moderator that everyone has decided
- Then, each commentator waits for the permission to speak for that question
- If they are given the permission, they do pthread_cond_timedwait on the breaking_news_event_happening condition
- If breaking news event does not happen during their speech, they inform the moderator that they are done speking
- If a breaking news event happens, they inform that they are cut short
- Moderator continues from the next commentator if a commentator is cut short due to a breaking news event
