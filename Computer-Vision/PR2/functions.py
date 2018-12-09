###################################
#                                 #
# Author: Ruiqi Wang              #
# Date: Dec 6th, 2018             #
#                                 #
###################################

import sys, cv2
import numpy as np
from matplotlib import pyplot as plt
import matplotlib.patches as patches

import tools

def Reduce(I):
    '''
    half down-sample of original input image

    param I: input image

    return: half down-sampled image
    '''
    reduced_index_x = [x for x in range(I.shape[0]) if not x % 2]
    reduced_index_y = [y for y in range(I.shape[1]) if not y % 2]
    I_reduced = I[reduced_index_x,:][:,reduced_index_y]
    return I_reduced

def Expand(I):
    '''
    up-sample of input image, and the size will be doubled on both width and height

    param I: input image

    return: up-sampled image
    '''
    I_expand = np.hstack((I,I))
    I_expand = np.vstack((I_expand, I_expand))
    I_expand = I_expand[[x/2 + x%2 * I.shape[0] for x in range(2*I.shape[0])],:]
    I_expand = I_expand[:,[y/2 + y%2 * I.shape[1] for y in range(2*I.shape[1])]]
    return I_expand

def GaussianPyramid(I,n):
    '''
    generate Gaussian pyramid on n levels

    param I: input image
    param n: levels of pyramid

    return: a list of image in Gaussian pyramid
    '''
    pyramid = [I]
    print "Generating Lv. 1 of Gaussian pyramid"
    for i in range(n-1):
        print "Generating Lv.", i+2,"of Gaussian pyramid"
        pyramid.append(cv2.pyrDown(pyramid[-1]))
    print
    return pyramid

def GetDisparity_region(img_l,
                        img_r,
                        kernel_size,
                        window_length,
                        measure,
                        disparity,
                        valid):
    if img_l.shape != img_r.shape:
        assert("Shape of two images are not matched")
    img_shape = img_l.shape
    height, width = img_shape[:2]
    img_l = tools.add_padding(img_l, kernel_size)
    img_r = tools.add_padding(img_r, kernel_size)
    padding_l = kernel_size / 2
    padding_r = kernel_size - padding_l
    padding_w = width + kernel_size
    if measure == "ssd":
        measure = cv2.TM_SQDIFF
    elif measure == "sad":
        measure = cv2.TM_SQDIFF_NORMED
    elif measure == "ncc":
        measure = cv2.TM_CCORR_NORMED

    shift = 0
    for y in range(padding_l, padding_l + height):
        for x in range(padding_l, padding_l + width):
            template = img_l[y - padding_l:y + padding_r, x - padding_l:x + padding_r]
            window_l, window_r = tools.get_window(x, padding_w, window_length)
            if not valid:
                window_l += disparity[y - padding_l,x - padding_l]
                window_r += disparity[y - padding_l,x - padding_l]
            else:
                window_l -= disparity[y - padding_l,x - padding_l]
                window_r -= disparity[y - padding_l,x - padding_l]
            window_l = max(0, window_l)
            window_r = min(padding_w, window_r)
            if window_r - window_l < kernel_size:
                # disparity[y - padding_l,x - padding_l] += shift
                continue
            window = img_r[y - padding_l:y + padding_r, window_l:window_r]
            values = cv2.matchTemplate(template, window, measure)
            minval,_, minloc,_ = cv2.minMaxLoc(values)
            if not valid:
                shift = minloc[0] - window_length / 2
            else:
                shift = window_length / 2 - minloc[0]
            disparity[y - padding_l,x - padding_l] += shift
    return disparity

def GetDisparity_feature(img_l,
                         img_r,
                         kernel_size,
                         feat_l,
                         feat_r,
                         measure,
                         disparity,
                         valid):
    pass
    if img_l.shape != img_r.shape:
        assert("Shape of two images are not matched")
    img_shape = img_l.shape
    height, width = img_shape[:2]
    img_l = tools.add_padding(img_l, kernel_size)
    img_r = tools.add_padding(img_r, kernel_size)
    padding_l = kernel_size / 2
    padding_r = kernel_size - padding_l
    padding_w = width + kernel_size
    if measure == "ssd":
        measure = cv2.TM_SQDIFF
    elif measure == "sad":
        measure = cv2.TM_SQDIFF_NORMED
    elif measure == "ncc":
        measure = cv2.TM_CCORR_NORMED
    feat_l = feat_l + padding_l
    feat_r = feat_r + padding_l

    for left_node in feat_l:
        minval = np.inf
        minloc = 0
        for right_node in feat_r:
            if left_node[0] > right_node[0]:
                continue
            if left_node[0] < right_node[0]:
                break
            block_l = tools.get_block(img_l, left_node, kernel_size)
            block_r = tools.get_block(img_r, right_node, kernel_size)
            val = cv2.matchTemplate(block_l, block_r, measure)[0][0]
            if val < minval:
                minloc = right_node[1] - left_node[1]
                minval = val
        if minval == np.inf or minloc < 0:
            continue
        left_node -= padding_l
        disparity[left_node[0], left_node[1]] += minloc
    return disparity