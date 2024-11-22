# ADVANTEDGE NFS PROTOCOL DOCUMENT

## *Confidential: For AdventEdge Internal Use Only*

## Message Format

All messages follow the format: `TYPE|COMMAND|PARAM1|PARAM2|...`

Where:
- TYPE: C (Client), N (Naming Server), S (Storage Server)
- COMMAND: Operation code
- PARAM(n): Additional parameters specific to each command

## Client Server Protocol

### Init Client:

Send: `C|V|<client_ip>|<client_port>`

Receive: `N|ACK` or `N|ERR`

### Client Operations:

Send: `C|<command>|<is_file>|<src_path>|<dst_path>`

Where:
- command: CREATE, COPY, DELETE, RENAME, STREAM
- is_file: 1 (file) or 0 (directory)
- src_path: Source path
- dst_path: Destination path (for COPY/RENAME operations)

Receive: Response format depends on operation:
- Success: `N|ACK|<additional_info>`
- Error: `N|ERR|<error_code>`

## Storage Server Protocol

### Init New Server:

Send: `S|V|-1|<ip>|<nm_port>|<dma_port>`

(Request ID From Server)

Receive: `N|<new_id>`

### Init Old Server:

Send: `S|V|<my_id>|<ip>|<nm_port>|<dma_port>`

(Move to Update Paths)

Receive: `N|ACK`

### Update Paths:

Send: `S|P|<my_id>|<path>`
Receive: `N|ACK`

### Download File (If Not Found):

// Act as Client READ

### File Operations:

1. CREATE:
   - Receive: `N|CREATE|<path>`
   - Send: 
      - `S|C_COMPLETE` (Success)
      - `S|C_NOTCOMPLETE` (Failure)

2. CREATE_DIRECTORY:
   - Receive: `N|CREATE_DIRECTORY|<path>`
   - Send: 
      - `S|CD_COMPLETE` (Success)
      - `S|CD_NOTCOMPLETE` (Failure)

2. DELETE:
   - Receive: `N|DELETE|<path>`
   - Send: 
      - `S|D_COMPLETE` (Success)
      - `S|D_NOTCOMPLETE` (Failure)

3. COPY:
   - Receive: `N|COPY|<src_path>|<dst_path>`
   - Send: 
      - `S|COPY_COMPELETE` (Success)
      - `S|COPY_NOTCOMPLETE` (Failure)

4. RENAME:
   - Receive: `N|RENAME|<old_path>|<new_path>`
   - Send: 
      - `S|RENAME_COMPLETE` (Success)
      - `S|RENAME_NOTCOMPELETE` (Failure)

5. STREAM:
   - Receive: `N|STREAM|<path>`
   - Send:
      - `S|STREAM_COMPLETE` (Success)
      - `S|STREAM_NOTCOMPLETE` (Failure)

6. READ:
   - Receive: `N|READ|<path>`
   - Send: `S|READ|<id>|<ip>|<dma_socket>`

   New Thread:
   - Receive: `C|READ|<path>`

      Loop:
      - Send: `S|READ|<data>`
      - Receive: `C|ACK`

   - Send: `S|READ|EOF`
   - Receive: `C|READ_COMPLETE|<bytes_read>` // To Client
   - Send: `S|READ_COMPLETE|<bytes_read>` // To Naming Server

   Failure:
   - Send: `S|READ_NOTCOMPLETE` // To Client and Naming Server

7. WRITE:
   - Receive: `N|WRITE|<path>`
   - Send: `S|WRITE|<id>|<ip>|<dma_socket>`

   New Thread:
   - Receive: `C|WRITE|<data>`

      Loop:
      - Receive: `C|WRITE|<data>`
      - Send: `S|ACK`
  
   - Receive: `C|WRITE|EOF`
   - Send: `S|WRITE_COMPLETE|<bytes_written>` // To Naming Server and Client

## Client Storage Server Protocol

###  Async Write Request:

Client Request:
`C|WRITE|<path>|<sync_flag>|<data_size>`

Storage Server Immediate Response:
`S|WRITE_ACK|<request_id>|<status>`

Async Write Completion:
`S|WRITE_COMPLETE|<request_id>|<status>|<bytes_written>`

Write Failure:
`S|WRITE_FAIL|<request_id>|<error_code>|<bytes_written>`

### Read Request:

Client Request:
`C|READ|<path>|<offset>|<length>`

Storage Server Response:
`S|READ_DATA|<status>|<data_length>|<data>`
or
`S|READ_LOCKED|<owner_id>|<expected_duration>`

### File Information Request:

Client Request:
`C|INFO|<path>`

Storage Server Response:
`S|INFO|<size>|<permissions>|<timestamps>|<owner>`

## Miscellaneous

### Backup Request:

Naming Server to Backup SS:
`N|REPLICATE|<source_ss_id>|<path>|<checksum>`

Backup SS Response:
`S|REPLICATE_ACK|<status>|<path>|<checksum>`

### Health Monitoring

Every 5 seconds, the Naming Server sends a PING message to each Storage Server, which responds with a PONG message.

Naming Server to SS:
`N|PING|<timestamp>`

Storage Server Response:
`S|PONG|<timestamp>|<load_status>|<free_space>`