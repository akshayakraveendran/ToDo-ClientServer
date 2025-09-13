To-Do List Client–Server Application

This project is a simple **real-time client–server To-Do List** application built using:

- **C++ (Server)** → Console app using Winsock2 for networking  
- **C# WPF (Client)** → .NET 8 desktop app with a clean GUI  

Multiple clients can connect to the server, manage tasks, and stay synchronized in real time.

## Project Structure

/Client → WPF .NET 8 client (UI)
/Server → C++ server (backend)
/todo.wpf.sln → Visual Studio solution
/README.md → Documentation
/.gitignore → Ignore build files

##  Requirements

- **Windows 10/11**
- **Visual Studio 2022** (or newer) with:
  - Desktop development with C++
  - .NET desktop development
- **.NET 8 SDK** (for WPF client)

---

##  Build Instructions

### 1. Build the Server
1. Open `To-do-WPF.sln` in Visual Studio.
2. Set **Server** as the Startup Project.
3. In Project → Properties → Linker → Input → Additional Dependencies, add:Ws2_32.lib
4. Build the project (`Ctrl+Shift+B`).

### 2. Build the Client
1. Set **Client** as the Startup Project.
2. Make sure `.csproj` targets:

<TargetFramework>net8.0-windows</TargetFramework>
<UseWPF>true</UseWPF>
3.Build the project (Ctrl+Shift+B)
Run the Server project (Ctrl+F5)
Console output:
Server listening on port 5000 ...

Start the Client
Run the Client project (Ctrl+F5).
WPF window opens and connects to the server.

Run Multiple Clients
In Visual Studio: Debug → Start New Instance
OR run Client.exe twice from:Client/bin/x64/Debug/net8.0-windows/

## Deliverable

1.Source code (Server + Client)

2.README.md (this file)

3.gitignore for clean repo

4.Visual Studio solution
