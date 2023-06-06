import cv2
import numpy as np
import math

row=256
col=256

f_ref=open("txt/ref_img.txt","r")
f_curr=open("txt/curr_img.txt","r")
f_rec=open("txt/rec_img.txt","r");
f_diff=open("txt/diff_img.txt","r");

ref_img=np.zeros((row,col,3),np.uint8)
curr_img=np.zeros((row,col,3),np.uint8)
diff_img=np.zeros((row,col,3),np.uint8)
rec_img=np.zeros((row,col,3),np.uint8)

for x in range(row*col):
	ref=f_ref.readline()#reference frame
	curr=f_curr.readline()#current frame
	data=f_diff.readline()#difference
	data1=f_rec.readline()#reconstructed
	r=int(math.floor(x/row))
	c=int(x-r*row)
	ref_img[r,c]=(np.uint8(ref),np.uint8(ref),np.uint8(ref))
	curr_img[r,c]=(np.uint8(curr),np.uint8(curr),np.uint8(curr))
	diff_img[r,c]=(np.uint8(data),np.uint8(data),np.uint8(data))
	rec_img[r,c]=(np.uint8(data1),np.uint8(data1),np.uint8(data1))
f_ref.close()
f_curr.close()
f_diff.close()
f_rec.close()
cv2.imwrite('imgs/ref_img.jpg',ref_img)
cv2.imwrite('imgs/curr_img.jpg',curr_img)
cv2.imwrite('imgs/diff_img.jpg',diff_img)
cv2.imwrite('imgs/rec_img.jpg',rec_img)
#image=cv2.imread('img.png')
#cv2.imwrite('img.jpg',image,[int(cv2.IMWRITE_JPEG_QUALITY),100])
