# Git450: A Simplified Git Management System

A distributed file management system that implements core Git-like functionality with multiple server components and TCP/UDP socket communication.

## 🌟 Features

- **Multi-Server Architecture**: A distributed system with four specialized servers
  - **Main Server (M)**: 
    - Core coordinator managing all communications between clients and other servers
    - Acts as a central hub - all messages must pass through this server
    - Handles both TCP (client communication) and UDP (inter-server communication)
    - Maintains connection state and routes messages appropriately

  - **Authentication Server (A)**:
    - Manages user authentication and credential verification
    - Maintains database of user login information
    - Verifies client credentials sent from main server
    - Supports both member and guest authentication modes

  - **Repository Server (R)**:
    - Manages file storage and repository operations
    - Maintains database of all user files and metadata
    - Handles file operations: lookup, push, and remove
    - Processes requests forwarded from main server
    
  - **Deployment Server (D)**:
    - Handles deployment operations for user repositories
    - Stores deployment history and status
    - Works in conjunction with Repository server for deployments
    - Manages deployment requests forwarded from main server

The system also includes:
- **Client Implementation**:
  - Provides user interface for all supported operations
  - Communicates with main server via TCP
  - Supports both member and guest user types
  
- **Utility Components**:
  - Password encryption functionality
  - Common data structures and helper functions
  - Shared utility functions used across servers

- **Secure Authentication**
  - Password encryption using cyclic offset algorithm
  - Support for member and guest user types
  - Session management and access control

- **Core Operations**
  - File lookup and discovery
  - Push/upload functionality
  - Deployment management 
  - File removal capabilities

## ⚙️ Architecture

```
                     TCP                 UDP
Client (Member) <-----------> Main Server <------------> Authentication Server (A)
                             (Server M)   <------------> Repository Server (R)
Client (Guest)  <----------->            <------------> Deployment Server (D)
                     TCP                      UDP

Communication Flow:
- Clients ←→ Main Server (TCP)
- Main Server ←→ Backend Servers (UDP)
- Backend Servers maintain database for their specific functions:
  * Server A: User credentials
  * Server R: File repository
  * Server D: Deployment records
```

The system follows a distributed architecture:
- Clients communicate with Main Server via TCP
- Inter-server communication uses UDP
- Authentication and file operations are isolated in separate servers
- All servers maintain persistent connections

## 🛠️ Technical Details

**Communication Protocols**:
- TCP for client-main server interaction
- UDP for inter-server communication
- Custom message protocols for different operations

**Security**:
- Custom encryption for passwords
- Access control based on user types
- Secure session management

**Storage**:
- File metadata management
- Repository storage system
- Deployment tracking

## 🚀 Getting Started

### Prerequisites
- Ubuntu 20.04
- GCC/G++ compiler
- Make utility

### Installation
1. Clone the repository
```bash
git clone [repository-url]
cd git450
```

2. Compile the project
```bash
make all
```

3. Start the servers (in order)
```bash
./start_all.sh
```

4. Stop the servers (in order)
```bash
./stop_all.sh
```

### Usage
Start client:
```bash
./client <username> <password>
```

Available commands:
- `lookup <username>`: View user's repository contents
- `push <filename>`: Upload file to repository 
- `deploy`: Deploy current repository
- `remove <filename>`: Remove file from repository

## 📁 Project Structure

```
/root
├── bin/                    # Executable files
├── data/                   # Configuration files
│   ├── filenames.txt      # Repository metadata
│   ├── members.txt        # User credentials
│   └── original.txt       # Reference data
├── src/                   # Source code
│   ├── client.c          # Client implementation
│   ├── serverM.c         # Main server
│   ├── serverA.c         # Authentication server
│   ├── serverR.c         # Repository server
│   ├── serverD.c         # Deployment server
│   ├── utils.c           # Utility functions
│   └── utils.h           # Header file
└── Makefile              # Build configuration
```

1. /src: all .c and .h files
2. /data: txt inputs
3. /bin: executable files after make all command will be in bin folder. please go to bin folder to test the project.

## 🔍 Notes
- Server startup order matters: M -> A -> R -> D -> Client
- All servers maintain persistent connections
- Uses TCP for client communications and UDP for inter-server messaging
- Based on principles from Beej's Guide to Network Programming

