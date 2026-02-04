#!/bin/bash
# Quick test script to run part3 multiple times and count results
# Run this from the code-examples directory

CORRECT=0
INCORRECT=0
EXPECTED="169315954.90"

for i in {1..20}
do
    RESULT=$(./part3_race_condition 2>&1 | grep "Actual total:" | awk '{print $3}')
    
    # Simple string comparison for exact match
    if [ "$RESULT" == "$EXPECTED" ]; then
        CORRECT=$((CORRECT + 1))
        echo "Run $i: CORRECT ($RESULT)"
    else
        INCORRECT=$((INCORRECT + 1))
        echo "Run $i: INCORRECT ($RESULT vs expected $EXPECTED) -- RACE DETECTED!"
    fi
done

echo ""
echo "==================================="
echo "Summary:"
echo "Correct:   $CORRECT / 20"
echo "Incorrect: $INCORRECT / 20"
if [ $INCORRECT -gt 0 ]; then
    echo ""
    echo "Race condition successfully demonstrated!"
    echo "$INCORRECT out of 20 runs had lost updates."
fi
echo "==================================="
