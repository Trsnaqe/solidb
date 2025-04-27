# SolidDB Storage and Persistence System

SolidDB uses a file-based storage system inspired by real database management systems to persist databases and tables on disk. This document describes how data is stored, loaded, and persisted.

## Storage Structure

### Directory Layout

- Each database is stored as a directory with the database name
- The database directory contains:
  - A `metadata.db` file with database metadata
  - One `.tbl` file for each table
  - A `.txlog` file that simulates a transaction log

```
mydb/                   # Database directory
├── metadata.db         # Database metadata
├── users.tbl           # Table file
├── products.tbl        # Another table file
├── orders.tbl          # Another table file
└── mydb.txlog          # Transaction log file
```

### Metadata File Format

The `metadata.db` file contains information about the tables in the database:

```
<number_of_tables>
<table_name_1>
<table_name_2>
...
```

For example:
```
3
users
products
orders
```

### Table File Format

Each table is stored in a separate `.tbl` file with the following format:

```
<table_name>
<number_of_columns>
<column1_name>,<column1_type>
<column2_name>,<column2_type>
...
<number_of_rows>
<row1_value1>,<row1_value2>,...
<row2_value1>,<row2_value2>,...
...
```

For example, a users table might look like:
```
users
3
id,INT
name,STRING
age,INT
2
1,John,25
2,Jane,30
```

### Transaction Log

The transaction log (`.txlog` file) records all write operations performed on the database. This mechanism is a simplified version of Write-Ahead Logging (WAL) used in production database systems.

## Durability and Persistence

SolidDB implements an industry-standard approach to data persistence inspired by real database systems:

### Write-Ahead Logging (WAL)

1. **Log First, Write Later**: When a user executes a command that modifies data:
   - The operation is first recorded in the transaction log
   - Then the change is applied to the in-memory database
   - The log provides a durable record of operations that can be replayed during recovery

2. **Operations Logged**:
   - Creating a new table
   - Inserting data into a table
   - Any other command that modifies the database state

3. **Implementation Status**:
   - The system is designed for WAL with the `logOperation` method and `wal_` vector in the Database class
   - Each operation is tracked with the `operationsSinceCheckpoint_` counter
   - Currently, the WAL implementation is defined in the interface but needs complete implementation in the code

### Transaction Management

SolidDB implements a simplified transaction model:

1. **Auto-Commit Mode**: 
   - By default, each statement is treated as a separate implicit transaction
   - Changes are held in memory until committed

2. **Explicit Commits**:
   - Users can issue a `COMMIT` command to explicitly save changes to disk
   - This forces all pending changes to be written to the database files

3. **Rollbacks**:
   - Users can issue a `ROLLBACK` command to discard changes since the last commit
   - This reloads the database state from disk, reverting to the last saved state

### Checkpoint System

To balance performance and durability, SolidDB implements a checkpoint mechanism:

1. **Periodic Checkpoints**: 
   - After every 5 write operations, a checkpoint is automatically triggered
   - During a checkpoint, all in-memory data is written to disk
   - Checkpoints are full database saves that create a consistent snapshot

2. **Manual Checkpoint/Commit**:
   - Users can force an immediate checkpoint with the `COMMIT` command
   - This ensures all changes are immediately written to disk

3. **Checkpoint on Exit**:
   - When the database system is shutting down, a final checkpoint ensures all changes are saved

4. **Implementation Status**:
   - The checkpoint system is integrated into the CommandParser class
   - The `operationCount` is tracked to trigger checkpoints every 5 operations
   - The `Database::checkpoint()` method is defined but currently needs complete implementation
   - Currently, checkpoints use the `saveToFile()` method for database persistence

### Saving Implementation

The database implements atomic saving with the following steps:

1. **Preparation**: Create the database directory if it doesn't exist
2. **Temporary Files**: Save all data to temporary files first (with .tmp extensions)
3. **Atomic Rename**: If all writes succeed, atomically rename temporary files to their final names
4. **Error Handling**: If any write fails, clean up temporary files to avoid corrupting existing data

This approach ensures data integrity even if the program crashes or loses power during a save operation.

### Loading Data

When loading a database, SolidDB:

1. Verifies the database directory exists
2. Loads the metadata file to identify tables
3. For each table mentioned in the metadata:
   - Loads the table file
   - Deserializes the table data
   - Adds the table to the in-memory database
4. Transaction logs can be replayed if needed during recovery

## Code Implementation

The persistence is implemented in:

1. **Database class**:
   - `saveToFile()` - Saves the database to disk
   - `loadFromFile()` - Loads a database from disk
   - `logOperation()` - Records operations to the WAL (pending implementation)
   - `checkpoint()` - Performs a full database save (pending implementation)

2. **Table class**:
   - `serialize()` - Converts a table to a string representation
   - `deserialize()` - Creates a table from its string representation

3. **CommandParser class**:
   - Tracks write operations
   - Maintains a counter for checkpointing
   - Handles `COMMIT` and `ROLLBACK` commands
   - Calls `saveToFile()` during checkpoints

## Future Improvements

Future versions of SolidDB will enhance the storage system with:

1. **Complete WAL implementation** - Finish implementing the Write-Ahead Logging system
2. **True incremental checkpoints**: Only save modified pages instead of the entire database
3. **Page-based storage**: Better performance for large datasets
4. **Full ARIES-style recovery**: Implement proper redo/undo logging with LSNs
5. **Two-phase commit**: Support for distributed transactions
6. **B+ Tree indexes**: Fast data access by indexed columns
7. **Buffer pool management**: Caching frequently accessed pages in memory
8. **Compression**: Reducing storage space requirements 
9. **Group commit**: Batching multiple transactions for efficient I/O 
10. **Recovery testing**: Ensuring the system can recover from crashes reliably 