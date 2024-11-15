README.txt
==========

Concurrent Hash Table Program
-----------------------------

This program implements a concurrent hash table with insert, delete, search, and print operations using reader-writer locks for synchronization. It reads commands from a file called `commands.txt` and writes output to `output.txt`.

### Files:

- **`chash.c`**: The main program containing all code.
- ** Makefile
- ** readme.txt

Program Behavior:

 . Input: The program reads commands from commands.txt. Each command specifies an operation (insert, delete, search, print) along with the necessary parameters.

 . Threads: The program creates a separate thread for each command, executing them concurrently.

 . Synchronization:

     .insert operations can proceed concurrently but are synchronized to ensure data integrity.
     .delete and search operations wait until all insert operations have completed.
     .Reader-writer locks are used to manage access to the hash table:
     .Writers (inserts and deletes) acquire a write lock.
     .Readers (searches and prints) acquire a read lock.

Output:

   .Logs: The program logs all operations with timestamps in output.txt, including lock acquisitions and releases.
   .Final Hash Table: After all commands have been processed, the program prints the final contents of the hash table to output.txt.
   .Lock Counts: The number of lock acquisitions and releases are tallied and displayed in output.txt.

AI Use Acknowledgment

    In developing this program, we utilized OpenAI's ChatGPT to assist with various aspects of the assignment. The AI assistance helped us in the following ways:

    .Clarifying Assignment Requirements: we sought explanations to better understand the specifications, particularly regarding the implementation of the print command and synchronization requirements.

    .Debugging Guidance: When encountering issues with lock counts and thread synchronization, we used the AI to gain insights into possible solutions and best practices.


    .Enhancing Output Clarity: we received recommendations on improving the logging output, such as adding separators to distinguish different sections in output.txt.

Prompts Used:

    . "How can I make the output of the print command more visible in my output.txt file?"

    . "What are some good practices for logging in a multithreaded application to distinguish different sections?"

    . "What is the expected behavior of the print,0,0 command in the context of this assignment?"

    . "How can I ensure that all lock acquisitions and releases are accurately counted in a multithreaded program?"

    . "What's the best way to organize my C code to maintain modularity while using a chash.c file?"

    . "How should synchronization be handled when multiple threads are performing insert, delete, search, and print operations?"