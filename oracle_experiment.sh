#!/bin/bash

victim_pid=6681
attack_pid=6682

echo "running oracle with attack"
sudo ./calibrate2_nv oracle_attack_log.csv

kill -9 $victim_pid
kill -9 $attack_pid

echo "running oracle without attack"
sudo ./calibrate2_nv oracle_idle_log.csv

