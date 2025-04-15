🐚 My Mini Shell – A Simple UNIX Shell in C

This project is a lightweight custom shell written in C that simulates basic terminal functionality found in Unix/Linux systems. It is designed for educational purposes and demonstrates key OS concepts like process creation, I/O redirection, piping, and signal handling.

⸻

✨ Features
	•	✅ Command execution (ls, cat, gcc, etc.)
	•	🔁 Command piping (|)
	•	🔄 Input/output redirection (<, >, >>)
	•	📜 Command history (up to last 10 commands)
	•	🚦 Signal handling (e.g., Ctrl+C gracefully prompts again)
	•	🧠 Built-in commands:
	•	exit — exits the shell
	•	history — shows recent commands

⸻

🔧 How It Works
	•	Uses fork() and execvp() for command execution
	•	Manages I/O with dup2() for redirection
	•	Handles piping by chaining multiple processes with pipe()
	•	Stores command history in a circular buffer
	•	Captures SIGINT (Ctrl+C) to avoid shell termination

⸻

🚀 Getting Started 
gcc -o my-mini-shell shell.c
./my-mini-shell

📚 Educational Value

This shell is a great starting point for understanding:
	•	Process control
	•	File descriptors and redirection
	•	System calls in Unix/Linux
	•	Shell parsing and command execution
