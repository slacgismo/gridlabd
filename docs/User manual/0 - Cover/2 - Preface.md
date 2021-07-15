# Preface

This manual is designed for general engineering, policy, regulatory, and economics readers who are interested in using GridLAB-D to model and simulate the behavior of modern electricity delivery systems.  Historically power system simulation tools were either solvers that could determine the power flow at a moment in time or the dynamic response for a short period of time, or they were simulators that were designed to produce verisimilar real-time behavior that is necessary for training operators and testing new hardware.

But the need to study and understand modern energy systems that are far more complex has given rise to a new generation of tools that can do more, including

 - simulate the evolution of the system over long periods of time,

 - bridge multiple technical domains that are not usually interacting in simulation tools,

 - collect large volumes of detailed data as the simulation progresses in time, and

 - permit users to quickly develop and implement new behaviors or new types of system devices and consumer equipment.

This is accomplished using what has become known as an agent-based model.  Agents are typically relatively simple objects that exhibit specific behaviors and interact with other agents in an environment that evolves over time, potentially in very complex ways.  

## Who needs this manual?

This manual is for model developers and simulation users.  It addresses the following topics:

 - Learning the GridLAB-D Modeling (GLM) language

 - Understanding and using GridLAB-D modules and classes

 - Optimizing your models and simulations

 - Exchanging models with other tools

 - Managing system performance

 - Administering and configuring GridLAB-D systems

 - Creating large-scale GridLAB-D operating environments

 - Understanding static modules and classes

 - Programming and debugging with GridLAB-D

 - Creating your own runtime classes

 - Installation and maintenance

The appendices contain the following

 - A thorough GLM language reference

 - References for Java, Matlab®, ODBC, gnutools, Microsoft® Visual Studio® and methods for interface GridLAB-D with other systems

## What not to expect from this manual

GridLAB-D is a very complex system and this manual covers everything you need to become a proficient GridLAB-D modeler and user.  However, it cannot explain everything. In particular, it does not contain the following topics:

 - How to program.  This manual assists you in programming GridLAB-D by providing reference information and examples, but it will not teach you how to program from the beginning.

 - How systems work in the real world.  This manual provides the information needed to represent real-world system, but it will not teach you how to engineer such systems, nor how they have been engineered.

 - How to compile and install software.  This manual will provide you information needed to develop and deploy your own GridLAB-D modules and packages, but it will not teach you how to use specific compilers, installation tools, and operating systems.

## What do you need?

You will need the following to work through the examples in this manual:

 - A copy of, or access to a GridLAB-D client or server.  You can download the latest version from the site `www.gridlabd.us`.

 - A system on which to install GridLAB-D (if you don’t have access to one already).  GridLAB-D can be installed on most desktop machines using [Docker](www.docker.com), but it can also be installed on dedicated servers, high-performance computers, and cloud platforms.

 - If you plan on developing add-ons and runtime classes, you may need to download the latest compilers and debuggers for your preferred development environment on your system.  GridLAB-D integrates primarily with Python and C++, but in principle you can use it in any programming environment.  Visit the GridLAB-D website at `www.gridlabd.us` to find out what the latest supported systems are.

## How to use this manual

This manual is divided into four parts.  If you’re new to agent-based modeling (ABM) and agent-based simulation (ABS), the best place to start is Part 1, "Fundamentals" and specifically Chapter 1, "Introduction", which will ease you into the world of GridLAB-D.  Readers that are experienced with other agent-based computing may want to give Chapter 1 a quick read just to get familiar with the way GridLAB-D represents and simulates things before exploring the GridLAB-D modules and classes in Chapter 2, Modeling.  Readers of an intermediate level can start with Chapter 3, “Advanced GridLAB-D” and Chapter 4, “Model Optimization”, which look into advanced modeling techniques and optimization.  Readers wanting to use a programming language to extend GridLAB-D should read Chapter 5, “Programming with GridLAB-D” and take a look at the appendix relevant to their programming language.  Chapter 6, “Extending GridLAB-D” is for those readers who understand GridLAB-D and want to add their own capabilities to it.

Readers without formal modeling and simulation knowledge will find Part II, “Designing GridLAB-D Models” very helpful.  This covers often overlooked issues that should be considered when developing large-scale simulations.

Any readers who wants to administer GridLAB-D systems will benefit from Part III, “Managing GridLAB-D Systems”, which addresses both basic installation and setup procedures and advanced issuers for optimizing high-performance systems.  It also explains system installation, backup, replication, and security issues.

Finally, the appendices are for detailed information on GridLAB-D programming language, runtime module documentation, function and operator references, as well as references to programming models and methods use by the most popular programming languages.

All the sample code is available for download at `code.gridlabd.us`.

## How to Contact the Authors

For errata, please check the website docs.gridlabd.us first.  Feel free to contact the authors with any questions or comments at gridlabd@gmail.com. They will do what they can to help out or point you in the right direction.
