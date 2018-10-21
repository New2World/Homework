###################################
#                                 #
# Author: Ruiqi Wang              #
# Date: Oct 20th, 2018            #
#                                 #
###################################

import cv2
import numpy as np
import matplotlib.pyplot as plt

def check_data_type(M):
    '''
    check data type, and transform to np.ndarray

    param M: input data type

    return: np.ndarray
    '''
    if isinstance(M, np.ndarray):
        return M
    return np.array(M)

def load_image(image):
    '''
    load image from file

    param image: path of image or image matrix in np.ndarray

    return: image matrix
    '''
    if isinstance(image,str):
        return cv2.imread(image,1)
    return image

def show_image(M):
    '''
    show image using matplotlib.pyplot

    param M: input image
    '''
    plt.imshow(cv2.cvtColor(M.astype('uint8'),cv2.COLOR_BGR2RGB))
    plt.show()

def add_padding(M, padding):
    '''
    add padding to input image with fixed width and height given by parameter padding

    param M: input image
    param padding: tuple of (width, height), give the width and height of padding

    return: image after padded
    '''
    # M = __check_data_type(M)
    if len(padding) > 1:
        vpadding_u = M[1:padding[1]+1,:,:][::-1,:,:]
        vpadding_d = M[-padding[1]-1:-1,:,:][::-1,:,:]
        M_vpadded = np.vstack((vpadding_u,M,vpadding_d))
    else:
        M_vpadded = M
    hpadding_l = M_vpadded[:,1:padding[0]+1,:][:,::-1,:]
    hpadding_r = M_vpadded[:,-padding[0]-1:-1,:][:,::-1,:]
    M_padded = np.hstack((hpadding_l,M_vpadded,hpadding_r))
    return M_padded

def get_im2col_indices(M_shape, kernel_shape):
    '''
    according to the convolution procedure, transfer the image to be convolved into a long matrix containing index of pixels in each slideing window.

    param M_shape: shape (including width, height and channel) of input image
    param kernel_shape: shape (including width and height) of kernel

    return: matrix of index of pixels in each slideing window
    '''
    H,W,C = M_shape
    kernel_height,kernel_width = kernel_shape
    out_height = H-kernel_height/2*2
    out_width = W-kernel_width/2*2

    i0 = np.tile(np.repeat(np.arange(kernel_height), kernel_width),C)
    i1 = np.repeat(np.arange(out_height), out_width)
    i = i0.reshape(-1, 1) + i1.reshape(1, -1)

    j0 = np.tile(np.arange(kernel_width), kernel_height * C)
    j1 = np.tile(np.arange(out_width), out_height)
    j = j0.reshape(-1, 1) + j1.reshape(1, -1)

    k = np.repeat(np.arange(C), kernel_height * kernel_width).reshape(-1, 1)

    return (i,j,k)

def im2col(M, kernel_shape):
    '''
    using the transferred index matrix to accelerate the convolution

    param M: input image
    param kernel_shape: shape (including width and height) of kernel
    '''
    if len(kernel_shape) > 1:
        kernel_height,kernel_width = kernel_shape
    else:
        kernel_height = 1
        kernel_width = kernel_shape[0]
    i, j, k = get_im2col_indices(M.shape, kernel_shape)
    cols = M[i,j,k].T
    cols = cols.reshape((-1,kernel_height*kernel_width))
    return cols.T

def affine_block(pos):
    '''
    build affine variable matrix

    param pos: one of points chose by user

    return: part of affine variable matrix
    '''
    block = np.array([[0,0,0,0,1,0],[0,0,0,0,0,1]], dtype=np.float32)
    block[0,0] = block[1,2] = pos[0]
    block[0,1] = block[1,3] = pos[1]
    return block

def affine_matrix(points):
    '''
    combine all parts of affine matrix into a large variable matrix of points

    param points: points chose by user in original image

    return: variable matrix of affine transfer
    '''
    return np.vstack(tuple(map(affine_block, points)))

def target_vector(points):
    '''
    vector of points' location in target image

    param points: points chose by user in target image

    return: vector of points
    '''
    pos_num = len(points)
    return np.hstack(points).reshape((pos_num * 2, 1))

def affine_point(point, parameter):
    '''
    according to the affine parameter, transform the point chose to be the base point in blending

    param point: point to be transformed
    param parameter: affine parameter

    return: point after transformation
    '''
    point = check_data_type(point)
    point = np.hstack((point,[1]))
    return parameter[0].dot(point.reshape((3,1))).flatten()

def ssd(img_1, img_2, point_1, point_2):
    '''
    compute the SSD for each points in both images

    param img_1, img_2: input images
    param point_1, point_2: points in both image

    return: SSD value of each pair of points
    '''
    def check_boundary(img, point):
        if point[0]-7 < 0 or img.shape[0] <= point[0]+7 or \
           point[1]-7 < 0 or img.shape[1] <= point[1]+7:
            return point, False
        return img[point[0]-7:point[0]+7,point[1]-7:point[1]+7], True
    def normalize(img):
        return img - np.mean(img)
    block_1, flag = check_boundary(img_1, point_1)
    if not flag:    return 1e10
    block_2, flag = check_boundary(img_2, point_2)
    if not flag:    return 1e10
    # print point_1, point_2
    # print block_1.shape, block_2.shape
    # if block_2.shape[0] == 13:
    #     print img_2.shape
    diff = normalize(block_1) - normalize(block_2)
    return np.sum(diff*diff)

def match_points(img_1, img_2, pos_1, pos_2):
    '''
    match points in both images from given point set

    param img_1, img_2: input images
    param pos_1, pos_2: point set

    return: matched points from both point set 1 and 2
    '''
    matchs_1 = []
    matchs_2 = []
    threshold = 30000
    prev_p1 = np.array([-100,-100])
    prev_p2 = np.array([-100,-100])
    for p1 in pos_1:
        if (prev_p1[0]-p1[0])**2+(prev_p1[1]-p1[1])**2 <= 25:
            continue
        pp = None
        pmin = 1e10
        for p2 in pos_2:
            if (prev_p2[0]-p2[0])**2+(prev_p2[1]-p2[1])**2 <= 25:
                continue
            d = ssd(img_1, img_2, p1, p2)
            if d < pmin:
                pmin = d
                pp = p2
        if pmin < threshold:
            matchs_1.append(p1[::-1])
            matchs_2.append(pp[::-1])
    return matchs_1, matchs_2

def print_corners(img, pos):
    '''
    mark corners in the image

    param img: input image
    param pos: corner points

    return: image with corners marked
    '''
    pos = check_data_type(pos)
    plt.imshow(cv2.cvtColor(img, cv2.COLOR_BGR2RGB))
    plt.scatter(pos[:,0].flatten(), pos[:,1].flatten(), marker='x', c='r')
    plt.show()
