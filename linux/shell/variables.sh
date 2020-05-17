#!/bin/bash

UserName=$1
UserAge=$2
echo "UserName: $UserName UserAge: $UserAge"

ThisScriptFileName=$0
echo "ThisScriptFileName: $ThisScriptFileName"

HowManyArgs=$#
echo "HowManyArgs: $HowManyArgs"

AllArgs=$@
echo "AllArgs: $AllArgs"

MostRecentProcessExitStatus=$?
echo "MostRecentProcessExitStatus: $MostRecentProcessExitStatus"

CurrentProcessID=$$
echo "CurrentProcessID: $CurrentProcessID"
