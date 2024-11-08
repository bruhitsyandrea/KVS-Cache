# KVS Cache

## 📚 Description
This project implements a **Key-Value Store (KVS) Cache** with a focus on optimizing caching mechanisms to enhance data retrieval speeds using various cache replacement policies. The KVS Cache supports multiple replacement strategies like **FIFO (First-In-First-Out)**, **LRU (Least Recently Used)**, and the **Clock Algorithm**, ensuring efficient memory usage and faster access times for large datasets.

### ⚠️ Disclaimer
> **Note**: This project was originally developed as part of a university course focused on computer systems. To prevent plagiarism and to respect academic integrity, the project has been renamed, and all references to specific course numbers and assignments have been removed. Please do not use this code for academic submissions.

## 🚀 Features
- **Multi-level Caching**: Implements FIFO, LRU, and Clock algorithms to optimize cache performance.
- **In-memory and Disk Storage**: Supports both memory and disk-based storage to enhance data retrieval efficiency.
- **Scalable Solution**: Efficiently handles large datasets with reduced latency.
- **Docker Support**: Easy setup using Docker for a consistent development environment.

## ⚙️ Installation
To set up the project locally, follow these steps:

## ⚙️ Running with Docker
This project was originally developed using a Docker environment provided as part of a university course. For privacy and academic integrity reasons, the original Docker setup (including `.devcontainer` and `.vscode` configurations) has been removed from this public repository. If you wish to set up a similar environment, please configure your own Dockerfile based on the project's requirements.

1. **Clone the repository**:
   ```bash
   git clone https://github.com/bruhitsyandrea/KVS-Cache.git
   cd KVS-Cache
2. **Compile the project**:
   ```bash
   make
3. **Run the KVS Cache**:
  ```bash
  ./kvs-cache
```
## 🛠 Usage
To use the KVS Cache, run the program with an input file containing commands:
```bash
./kvs-cache input.txt
```
## Example Input (input.txt):
```sql
SET key1 value1
GET key1
SET key2 value2
DEL key1
GET key2
```
## Example Output:
```sql
Value for key1: value1
Deleted key1
Value for key2: value2
```
## 🗂 Project Structure
```css
KVS-Cache/
├── .vscode/
│ └── .clang-format
├── .gitignore
├── Makefile
├── README.md
├── client.c
├── constants.h
├── kvs.c
├── kvs.h
├── kvs_base.c
├── kvs_base.h
├── kvs_clock.c
├── kvs_clock.h
├── kvs_fifo.c
├── kvs_fifo.h
├── kvs_lru.c
└── kvs_lru.h
```
### Explanation:
1. **`.vscode/`**: Contains the `.clang-format` file for code formatting.
2. **Source Code Files**: Includes `client.c`, `kvs.c`, `kvs_base.c`, etc.
3. **Headers Files**: Contains corresponding `.h` files for the source code.

## 📈 Optimization Strategies
This project explores different cache optimization techniques:

    FIFO: Removes the oldest data first.
    LRU: Removes the least recently accessed data.
    Clock: Approximates LRU with lower overhead, providing an efficient replacement strategy.
## 🛠️ Technologies Used
Languages: C, C++
Tools: Makefile, Git, Docker
Operating Systems: Linux, macOS
## 📬 Contact
Andrea Chen
Email: ychen729@ucsc.edu
GitHub: bruhitsyandrea


