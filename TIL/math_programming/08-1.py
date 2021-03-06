import cv2
import numpy as np
img=cv2.imread('C:\\Users\\chaeh\\workspace3\\image.jpg',cv2.IMREAD_COLOR)
height,width=img.shape[:2]
scale_factor=0.5
scaling_matrix=np.array([[scale_factor,0,0],[0,scale_factor,0],[0,0,1]])
dst=np.zeros((height,width,img.shape[2]), dtype=np.uint8)
for y in range(height):
    for x in range(width):
        new_p=np.array([x,y,1])
        inv_scaling_matrix=np.linalg.inv(scaling_matrix)
        old_p=np.dot(inv_scaling_matrix,new_p)
        x_,y_ = old_p[:2]
        x_=int(x_)
        y_=int(y_)
        if x_>0 and x_<width and y_ >0 and y_<height:
                dst.itemset((x,y,0),img.item(y_,x_,0)) #blue channel
                dst.itemset((x,y,1),img.item(y_,x_,1)) #green channel
                dst.itemset((x,y,2),img.item(y_,x_,2)) #red channel
result=cv2.hconcat([img,dst])
cv2.imshow("result",result)
cv2.waitKey(0)
