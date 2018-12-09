###################################
#                                 #
# Author: Ruiqi Wang              #
# Date: Dec 6th, 2018             #
#                                 #
###################################

import cv2
import numpy as np
import matplotlib.pyplot as plt

# define Gaussian kernel
Gaussian_kernel2D = np.array([[1, 4, 6, 4,1],
                              [4,16,24,16,4],
                              [6,24,36,24,6],
                              [4,16,24,16,4],
                              [1, 4, 6, 4,1]],
                              dtype=np.float32)

# define mean kernel
mean_kernel2D = np.ones((5,5)) / 25.

def check_data_type(M):
    '''
    check data type, and transform to np.ndarray

    param M: input data type

    return: np.ndarray
    '''
    if isinstance(M, np.ndarray):
        return M
    return np.array(M)

def load_image(image, mode):
    '''
    load image from file

    param image: path of image or image matrix in np.ndarray

    return: image matrix
    '''
    if isinstance(image,str):
        return cv2.imread(image,mode)
    return image

def show_image(M, n=0):
    '''
    show image using matplotlib.pyplot

    param M: input image
    '''
    if isinstance(M, list):
        n = len(M)
        for i in range(n):
            plt.subplot(n,1,i+1)
            if len(M[i].shape) > 2 and M[i].shape[-1] > 1:
                plt.imshow(cv2.cvtColor(M[i].astype('uint8'),cv2.COLOR_BGR2RGB))
            else:
                plt.imshow(M[i].astype('uint8'), cmap="gray")
    else:
        if len(M.shape) > 2 and M.shape[-1] > 1:
            plt.imshow(cv2.cvtColor(M.astype('uint8'),cv2.COLOR_BGR2RGB))
        else:
            plt.imshow(M.astype('uint8'), cmap="gray")
    plt.show()

def add_padding(M, padding, mode="mirror"):
    '''
    add padding to input image with fixed width and height given by parameter padding

    param M: input image
    param padding: tuple of (width, height), give the width and height of padding

    return: image after padded
    '''
    padding /= 2
    if mode == "mirror":
        vpadding_u = M[1:padding+1,:][::-1,:]
        vpadding_d = M[-padding-1:-1,:][::-1,:]
        M_vpadded = np.vstack((vpadding_u,M,vpadding_d))
        hpadding_l = M_vpadded[:,1:padding+1][:,::-1]
        hpadding_r = M_vpadded[:,-padding-1:-1][:,::-1]
        M_padded = np.hstack((hpadding_l,M_vpadded,hpadding_r))
    elif mode == "zero":
        h, w = M.shape[:2]
        vpadding = np.zeros((padding,w))
        M_vpadded = np.vstack((vpadding,M,vpadding))
        hpadding = np.zeros((h+padding*2,padding))
        M_padded = np.hstack((hpadding,M_vpadded,hpadding))
    return M_padded.astype(np.uint8)

def get_features(img):
    feat = cv2.dilate(cv2.cornerHarris(img, 3, 5, .05), None)
    feat = np.array(np.nonzero(feat>.1*feat.max())).T
    return feat

def get_block(img, point, kernel_size):
    kernel_l = kernel_size / 2
    kernel_r = kernel_size - kernel_l
    y, x = point
    return img[y - kernel_l:y + kernel_r,x - kernel_l:x + kernel_r]

def get_window(x, w, l):
    ll = l / 2
    lr = l - ll
    window_l = max(0, x - ll)
    window_r = min(w, x + lr)
    return window_l, window_r