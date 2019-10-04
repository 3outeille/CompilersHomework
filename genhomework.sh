#!/bin/sh
HWNUM=$1
PDFIN=${HWNUM}.pdf
FILE=hw${HWNUM}.md
echo "Convert $PDFIN"
trash $FILE
echo "# HW${HWNUM}" >> $FILE
echo "" >> $FILE
pdftoppm $PDFIN $HWNUM -png
for i in `ls ${HWNUM}*.png`; do
	echo "![](./$i)" >> $FILE
done

