# Image Database
---
**NOTE:** This repository is no longer maintained.
Due to serious design flaws and bugs, this project has been abandoned in favour of [IDB NG](https://github.com/UnprofessionalProfessional/IdbNG).

# Building
---
If you really want to build this project,
1. Download and install PostgreSQL 16 for Windows.
2. Install Microsoft Visual Studio 2022 (other version have not been tested)  
   i. Install the "Desktop development with C++" package
3. Create a database called "idb_%USERNAME%" where `%USERNAME%` is the result of `echo %USERNAME%` in Command Prompt.
4. After logging into the database via `psql.exe`, copy and paste the contents of `schema.sql` into the SQL prompt, then exit.
5. Open the solution and build it.
6. Run the programme.
7. Hopefully it runs
