TriangleOS — Vision

1. Purpose
TriangleOS is an independent operating system being written from first-principles for x86-64 computers.
TriangleOS is not a Linux, Unix or modified existing operating system. The purpose is to write the major components of the system from scratch:

Bootloader
Kernel
Hardware interfaces
Memory management
Process and task management
Filesystem
User environment
Compiler
Package format
Package manager
Repository infrastructure
System applications

The project will attempt to explore what an operating system can be, when its architecture is designed as a whole rather than to come from an existing operating-system ecosystem.

2. Architecture
TriangleOS targets x86-64 only.
The project will not target ARM, RISC-V or other CPUs.
This allows the system to focus its engineering effort on one architecture and to use its capabilities, rather than maintaining a large collection of portability layers.

3. Independence
TriangleOS should not depend on the Linux or Unix ecosystems at runtime.
The final operating system should not require things such as:

Linux
A Linux kernel
GNU/Linux userland
systemd
POSIX
Unix system calls
Linux package formats
Linux repositories
Linux filesystem conventions

Development tools such as Git, QEMU, assemblers, compilers and other software may be used for building TriangleOS, but are development dependencies, not dependencies of the finished operating system.
The long-term goal is for TriangleOS to provide its own system environment from the boot process upwards.

4. Different Operating-System Model
TriangleOS should not simply reproduce traditional Unix concepts under different names.
Its architecture should be designed around concepts that are useful for TriangleOS itself.
Possible foundational concepts to explore include:

Objects
Capabilities
Tasks
Events
Resources
Memory regions
Services
Hardware devices

These concepts are not required, but areas to investigate during development.
The important principle is:

> TriangleOS should implement concepts because they are useful, not because existing operating systems traditionally implement them.

6. Performance
Performance is a major design goal.
TriangleOS should be capable of running on both older and modern x86-64 computers without unnecessary overhead.
Performance should come from architecture rather than from simply omitting features.
The system should aim for:

Low boot overhead
Low idle memory usage
Efficient scheduling
Efficient memory management
Minimal unnecessary background services
Fast program startup
Efficient filesystem operations
Native machine-code execution
Performance must not come at the cost of making the system impossible to maintain or develop.

7. Package Ecosystem
TriangleOS will have its own software distribution system.
The project should eventually provide:

A Triangle package format
A package manager
Official repositories
Community repositories
Package metadata
Dependency management
Package signing
Version management
Reproducible builds where practical

TriangleOS packages should not depend on Linux package formats such as `.deb` or `.rpm`.
The package ecosystem should be designed specifically for TriangleOS.

8. Security
Security should be considered part of the architecture, rather than something added after the operating system is finished.
Areas of interest include:

Capability-based access
Memory isolation
Process isolation
Controlled hardware access
Permission boundaries
Secure package verification
Signed software
Safe system interfaces
Security mechanisms should be understandable and auditable where practical.

9. User Experience
TriangleOS should eventually provide a complete graphical environment, but the graphical interface is not the foundation of the project.
The system should first become a functional operating system through a reliable low-level foundation.
The eventual environment should aim to be:

Fast
Simple
Consistent
Responsive
Customizable
Native to TriangleOS

The command-line environment should remain a first-class part of the system.

10. Development Philosophy
TriangleOS will be developed incrementally.
Each stage should produce something that can be tested independently.
The approximate progression is:

text
Boot
↓
Kernel
↓
CPU initialization
↓
Interrupts
↓
Memory management
↓
Hardware input
↓
Timers
↓
Tasks
↓
Filesystem
↓
Programs
↓
Package system
↓
User environment
↓
Graphical environment

The exact architecture may change as the project develops.
Changing an early design because testing proves it to be wrong is a feature of the development process, not a failure.

11. Testing
TriangleOS should be tested primarily in virtual machines during early development.
QEMU will be used to provide a safe and repeatable development environment.
Physical hardware testing will be introduced gradually after the basic system has become stable.
The project should eventually test against different generations of x86-64 hardware to ensure that its hardware assumptions are intentional and documented.

12. Source Availability
TriangleOS source code will be publicly available.
The project is intended to allow people to:

Study the source
Modify it
Create forks
Experiment with the system
Build their own versions
Contribute improvements

TriangleOS is licensed under the PolyForm Noncommercial License 1.0.0.
Commercial use requires permission from the applicable copyright holder(s).
See `LICENSE` for the complete license terms.

13. Long-Term Goal
The long-term goal is a complete, independently designed x86-64 operating system that can:

1. Boot directly on compatible hardware.
2. Manage CPU, memory, devices and storage.
3. Run multiple programs safely.
4. Provide its own filesystem and system interfaces.
5. Compile and run Triangle programs.
6. Install and update software through Triangle's package ecosystem.
7. Provide a complete command-line environment.
8. Eventually provide a native graphical environment.
9. Become increasingly self-hosting.
10. Exist as a coherent system rather than a collection of components inherited from another operating system.

TriangleOS is an experiment in building an operating system from the foundations upwards.
The project will prioritise understanding, independence, performance, simplicity and experimentation over compatibility with existing operating-system conventions.
