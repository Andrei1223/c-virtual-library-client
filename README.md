# Virtual Library Client

## Important Notice

The server previously configured for this project, located at the IP address
`34.246.184.49` on port `8080`, is no longer active. The client will not be able
to establish a successful connection unless the `SERVER_ADDR` and `PORT` macros in
the source code are updated to point to an active server.


## Project Description

This project is a Command Line Interface (CLI) HTTP client written in C. It was designed
to function as an interface for a virtual library, sending specific HTTP requests to a RESTful
server based on user input from the standard input (stdin).

## Architecture and Components

The application is modularized into several components to handle different layers of
the communication process:

* **Main Client Logic**: The main loop parses user input and routes the commands to the
appropriate functions.
* **Request Formatting**: Functions are implemented to compute and format raw HTTP `GET`
and `POST` requests, including headers, cookies, and JWT authorization.
* **Socket Communication**: Helper functions manage the lower-level socket operations,
including opening connections, transmitting data, and receiving raw HTTP responses.
* **Dynamic Buffering**: A dynamic buffer structure is utilized to safely read and store
incoming HTTP response streams of varying sizes.


## Supported Commands

The client supports a continuous prompt that accepts the following commands:

* **`register`**: Creates a new user in the server's database. It prompts the user
to input a username and password via stdin.
* **`login`**: Authenticates the user. Upon success, the server returns a session cookie 
used to prove account access.
* **`enter_library`**: Requests access to the virtual library using the authentication
cookie. The server responds with a JWT token necessary for authorizing subsequent library-related requests.
* **`get_books`**: Retrieves all books saved in the user's library. The client prints the
resulting JSON data exactly as received from the server.
* **`get_book`**: Fetches the details of a specific book. It requires the user to input the
specific book ID.
* **`add_book`**: Adds a new book to the database. It prompts the user for the title, author,
genre, publisher, and page count.
* **`delete_book`**: Deletes a book from the server. It prompts the user for the ID of the book 
to be removed.
* **`logout`**: Logs the user out of the server, invalidating the current session tokens.
* **`exit`**: Closes the client application safely.


## Error Handling

The client implements local validation and processes HTTP status codes to handle various error states:

* **Registration Errors**: Fails if the chosen username is already taken. Registration is also blocked locally if the username contains spaces or numerical digits.
* **Authentication Errors**: Login fails if the username does not exist or the provided password is incorrect.
* **Authorization Errors**: Library commands (fetching, adding, or deleting books) will fail if the user is not authenticated or has not requested library access.
* **Input Validation**: Adding or deleting books includes local checks to ensure inputs like page counts and IDs contain only valid numeric characters. Operations fail gracefully if a requested book ID does not exist.


## Building and Running

The project includes a `Makefile` to simplify the compilation process using the GCC compiler.

1. **Compile the client**: Run `make client` or simply `make` to build the executable.
2. **Run the client**: Run `make run` to start the compiled application.
3. **Clean the directory**: Run `make clean` to remove the compiled executable and any object files.
