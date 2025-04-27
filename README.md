# SolidDB - A Simple Relational Database in C++

SolidDB is a barebones relational database management system implemented in C++ for educational purposes. This project is being developed as a graduation project to demonstrate understanding of database systems concepts.

## Current Features

- Database creation and management
- Table creation with simple column types
- Column constraints (PRIMARY KEY, UNIQUE, NOT NULL)
- Basic INSERT operations to add data to tables
- Basic SELECT operations with simple WHERE conditions
- **Write-Ahead Logging** with periodic checkpoints for durability
- Transaction management with COMMIT and ROLLBACK

## Planned Features

- B+ Tree indexing for efficient data access
- Concurrency control with latches and locks
- Advanced query processing
- SQL parser for more complex queries
- Buffer pool management

## Prerequisites

- C++ compiler with C++17 support (GCC 7+ or Clang 5+)
- CMake 3.10 or higher
- On Ubuntu/Debian, install with: `sudo apt install cmake g++`

## Building the Project

The project uses CMake as its build system:

```bash
# Create a build directory
mkdir build
cd build

# Generate build files
cmake ..

# Build the project
make
```

## Usage

After building, you can run the SolidDB executable:

```bash
./soliddb
```

This will start the interactive CLI where you can enter commands.

### Example Commands

```sql
-- Create a new database
CREATE DATABASE mydb

-- Use the database
USE mydb

-- Create tables
CREATE TABLE users (id INT PRIMARY KEY, name STRING NOT NULL, email STRING UNIQUE)
CREATE TABLE products (id INT PRIMARY KEY, name STRING NOT NULL, price INT)

-- Insert data
INSERT INTO users VALUES (1, John, john@example.com)
INSERT INTO users VALUES (2, Jane, jane@example.com)

-- Save changes to disk
COMMIT

-- Insert more data
INSERT INTO users VALUES (3, Bob, bob@example.com)

-- Query data
SELECT * FROM users

-- Roll back to last commit
ROLLBACK

-- Query again - only the committed data remains
SELECT * FROM users
```

## Data Persistence

SolidDB implements industry-standard durability mechanisms similar to real database systems:

### Write-Ahead Logging

- Each write operation is first logged (simulating a transaction log)
- This provides disaster recovery capability in production databases

### Transaction Management

- Changes are kept in memory until explicitly committed
- Use `COMMIT` to save all changes to disk
- Use `ROLLBACK` to revert to the last committed state

### Checkpoint System

- Every 5 write operations, the database performs a checkpoint
- Checkpoints persist the current state to disk
- This balances performance with durability

### Storage Format

Data is stored in a structured format:
- Each database is a directory
- Database metadata is stored in a `metadata.db` file
- Each table is stored in its own `.tbl` file

For more details on the storage format and implementation, see [Storage Documentation](docs/Storage.md).

## Project Structure

- `include/` - Header files
  - `core/` - Core database classes
  - `parser/` - SQL parser
  - `util/` - Utility functions
- `src/` - Source files
  - `core/` - Implementation of core components
  - `parser/` - Implementation of parser components
  - `util/` - Implementation of utility functions
- `docs/` - Documentation
- `tests/` - Test files (to be added)

## License

This project is created for educational purposes and is not intended for production use.