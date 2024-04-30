#!/bin/bash

function gen_input() {
	local max=${1:-100}
	local interval=${2:-1}
	local money=${3:-4000}
	local player=4

	echo start
	echo $money
	echo $player
	echo 1
	echo 2
	echo 3
	echo 4

	for ((i = 0; i < max; i++)); do
		sleep $interval
		echo roll
		echo y
		echo 1
	done

	echo quit
}

make

gen_input 100 0.4 | ./monopoly

