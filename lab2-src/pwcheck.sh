#!/bin/bash

#DO NOT REMOVE THE FOLLOWING TWO LINES
#git add $0 >> .local.git.out
#git commit -a -m "Lab 2 commit" >> .local.git.out
#git push >> .local.git.out || echo

#DEF testfile 
  testfile=$1
#Your code here
STRING="$(cat $testfile)"
#Password has less than 6, or more than 32 characters
if [ ${#STRING} -lt 6 ] || [ ${#STRING} -gt 32 ]; 
then
	echo Error: Password length invalid.
	exit 1
fi

#Print "Error: Password length invalid."

#For any valid password:

#+1 point for each character in the string
#echo STRING $STRING
SCORE=${#STRING}
#echo String $SCORE

#If the password contains one of the following special characters (#$+%@)
if [[ $STRING = *[#$+%@]* ]] ; then
	let SCORE=SCORE+5
#	echo special $SCORE
fi

#+5 points

#If the password contains at least one number (0-9)

if egrep [0-9] $testfile > out.txt;then
	let SCORE=SCORE+5
	#echo 0-9 $SCORE
fi



#+5 points

#If the password contains at least one alpha character (A-Za-z)
if egrep [A-Za-z] $testfile > out.txt;then
	let SCORE=SCORE+5
	#echo char $SCORE
fi

#echo print before deduction $SCORE
#initialize number of times against rules
COUNT_REPEAT=0
COUNT_LOW=0
COUNT_UPP=0
COUNT_NUM=0

#+5 poinits

for((i=0;i<${#STRING};i++))
do
	CHAR=1
#If the password contains a repeated alphanumeric character (i.e. aa, bbb, 55555)
	if [ "${STRING:i:CHAR}" == "${STRING:i+1:CHAR}" ]; then
		let COUNT_REPEAT=COUNT_REPEAT+1
	fi

	CON=3

#-10 points

#If the password contains 3 or more consecutive lowercase characters (i.e. bbb, abe, this)
#translate uppper to lower
	lower=$(tr '[:upper:]' '[:lower:]' <<< ${STRING:i:CON} )
	if [[ ${STRING:i:CON} =~ ^[a-z]+$ ]] && [[ ${#lower} -eq 3 ]] && [ "${STRING:i:CON}" == "$lower" ]; 
	then
		let COUNT_LOW=COUNT_LOW+1
	fi

#-3 points

#If the password contains 3 or more consecutive uppercase characters (i.e. BBB, XQR, APPLE)

	upper=$(tr '[:lower:]' '[:upper:]' <<< ${STRING:i:CON})
	if [[ ${STRING:i:CON} =~ ^[A-Z]+$ ]] && [[ ${#upper} -eq 3 ]] && [  "${STRING:i:CON}" == "$upper" ];
	then
		let COUNT_UPP=COUNT_UPP+1
	fi

	if [[ ${STRING:i:CON} =~ ^[0-9]+$ ]] && [[ $i -lt ${#STRING}-2 ]];
	then
		let COUNT_NUM=COUNT_NUM+1
	fi
done


if [ $COUNT_REPEAT -gt 0 ]; then
	let SCORE=SCORE-10
fi

if [ $COUNT_LOW -gt 0 ]; then
	let SCORE=SCORE-3
fi

if [ $COUNT_UPP -gt 0 ]; then
	let SCORE=SCORE-3
fi

if [ $COUNT_NUM -gt 0 ]; then
	let SCORE=SCORE-3
fi
#-3 points

#If the password contains 3 or more consecutive numbers (i.e. 55555, 1747, 123, 321)

#-3 points


echo Password Score: $SCORE






















