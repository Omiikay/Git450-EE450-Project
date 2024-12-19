Ubuntu Version:
- Ubuntu 20.04 (from provided docker)

File structure

├── bin
│   ├── client
│   ├── serverA
│   ├── serverD
│   ├── serverM
│   ├── serverR
├── data
│   ├── filenames.txt
│   ├── members.txt
│   ├── original.txt
├── src
│   ├── client.c
│   ├── serverA.c
│   ├── serverD.c
│   ├── serverM.c
│   ├── serverR.c
│   ├── utils.c
│   └── utils.h
├── readme.txt
└── Makefile

1. /src: all .c and .h files
2. /data: txt inputs
3. /bin: executable files after make all command will be in bin folder. please go to bin folder to test the project.


a. Your Full Name as given in the class list
- Zouyan Song

b. Your Student ID
- 1171340446

c. What you have done in the assignment, if you have completed the optional part (suffix). If it’s not mentioned, it will not be considered.
- Only mandatory commands (no log).


d. What your code files are and what each one of them does. (Please do not repeat the project description, just name your code files and briefly mention what they do).
- serverM.c 
 Implementation of main server. It manages all the communications between client and other servers, forwards messages between them. 
 Client and other back servers cannot connect with each other directly, must go through this main server to communicate with each other.
 
- serverA.c
 Implementation of authentication server. It serves as a database, storing login infomatoin of all the users.
 The main server will send a request to serverA to find if the login info from client matches records in it.
 
- serverR.c
 Implementation of Repository server. It serves as another database, storing all users files.
 Client could send a request to review or manipulate those files via main server.

- serverD.c
 Implementation of Deployment server. It also serves as a database, storing all the deployment behavior of users.
 When client send a deploy request the main server will forward it to serverR and serverD, if any available file found in this client's repository, 
 the main server will request to serverD and 'deploy' those data in serverD.

- client.c
 Implementation of client. It provides all the behaviors supported in the system. Client could send request to main server via TCP and get response from it.

- utils.c, utils.h 
 utils.c & utils.h contains utility functions for password encryption and some predefined struct used in other files. 


e. The format of all the messages exchanged, e.g., usernames are concatenated and delimited by a comma, etc.

1. Authentication
	1) client->serverM: .client <username> + " " + <password>
	
2. Commands
	1) client->serverM: .client <command> <option>


g. Any idiosyncrasy of your project. It should say under what conditions the project fails, if any.
- Input commands not supported.


h. Reused Code: Did you use code from anywhere for your project? If not, say so. If so, say what functions and where they're from. 
 (Also identify this with a comment in the source code). Reusing functions which are directly obtained from a source on the internet without or with few modifications is considered plagiarism (Except code from the Beej’s Guide). 
 Whenever you are referring to an online resource, make sure to only look at the source, understand it, close it and then write the code by yourself.
 1. getsockname() function to retrieve the locally-bound port number wherever ports are assigned dynamically.
 2. functions like: bind(), listen(), send(), recv() functions are modified from Beej's Guide to Network Programming, used in serverM.c, serverA.c, serverR.c, serverD.c, client.c.