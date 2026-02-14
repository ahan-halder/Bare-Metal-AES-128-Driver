# Bare-Metal-AES-128-Driver
Bare-Metal C Driver and Software Stack for an AES Hardware Accelerator

## Introduction
Modern System-on-Chip (SoC) development requires close coordination between hardware design and 
low-level software, especially during silicon bring-up and IP validation. This project aims to simulate a 
real-world software-for-silicon workflow by designing and validating a bare-metal C software stack for an 
AES-128 hardware accelerator. 
Unlike application-level cryptography projects, this work focuses on register-level control, 
memory-mapped I/O (MMIO), and hardware-software co-design. The AES-128 RTL module is not 
implemented initially and will be developed during the project, followed by integration with a minimal 
firmware driver. The final system will resemble an early-stage silicon environment where firmware is written 
to control and validate a newly developed cryptographic IP core.

## Objectives
The key objectives of this project are: 
●  To design and implement an AES-128 encryption hardware accelerator in RTL 
●  To define a memory-mapped register interface for hardware control and status monitoring 
●  To develop a bare-metal C driver for interacting with the AES hardware 
●  To validate correct hardware-software operation using standard AES test vectors 
●  To gain practical experience in SoC firmware development and silicon bring-up workflows
