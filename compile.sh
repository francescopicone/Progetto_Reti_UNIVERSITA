#!/bin/bash

gcc Client_Studente.c Common/protocol.c -o Client_Studente
gcc Server_Segreteria.c Common/protocol.c -o Server_Segreteria

