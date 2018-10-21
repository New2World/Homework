###################################
#                                 #
# Author: Ruiqi Wang              #
# Date: Oct 20th, 2018            #
#                                 #
###################################

import sys, cv2
import numpy as np
from matplotlib import pyplot as plt

import tools

# define Gaussian kernel
Gaussian_kernel2D = np.array([[1, 4, 6, 4,1],
                              [4,16,24,16,4],
                              [6,24,36,24,6],
                              [4,16,24,16,4],
                              [1, 4, 6, 4,1]],
                              dtype=np.float32)

def Convolve(I,K):
    '''
    convolution

    param I: input image
    param K: kernel

    return: image after convolution
    '''
    I = tools.check_data_type(I)
    K = tools.check_data_type(K)
    kernel_shape = K.shape
    kernel_dim = K.ndim
    padding_tup = None
    if kernel_dim == 1:
        padding_tup = (kernel_shape[0]/2,)
    else:
        padding_tup = (kernel_shape[0]/2,kernel_shape[1]/2)
    I_padded = tools.add_padding(I, padding_tup)
    I_cols = tools.im2col(I_padded,K.shape)
    I_dash = K.flatten().dot(I_cols)
    I_dash = I_dash.reshape(I.shape)
    return I_dash

def Reduce(I):
    '''
    half down-sample of original input image

    param I: input image

    return: half down-sampled image
    '''
    I_blurred = Convolve(I,Gaussian_kernel2D)
    reduced_index_x = [x for x in range(I.shape[0]) if not x % 2]
    reduced_index_y = [y for y in range(I.shape[1]) if not y % 2]
    I_reduced = I_blurred[reduced_index_x,:,:][:,reduced_index_y,:]
    return I_reduced / 256.

def Expand(I):
    '''
    up-sample of input image, and the size will be doubled on both width and height

    param I: input image

    return: up-sampled image
    '''
    I_expand = np.zeros((I.shape[0]*2,I.shape[1]*2,I.shape[2]))
    I_expand[:I.shape[0],:I.shape[1],:] = I
    I_expand = I_expand[[x/2 + x%2 * I.shape[0] for x in range(2*I.shape[0])],:,:]
    I_expand = I_expand[:,[y/2 + y%2 * I.shape[1] for y in range(2*I.shape[1])],:]
    I_expand_blur = Convolve(I_expand,Gaussian_kernel2D)
    return I_expand_blur / 64.

def GaussianPyramid(I,n):
    '''
    generate Gaussian pyramid on n levels

    param I: input image
    param n: levels of pyramid

    return: a list of image in Gaussian pyramid
    '''
    pyramid = [I]
    print "    generating Lv. 1 of Gaussian pyramid"
    for i in range(n-1):
        print "    generating Lv.", i+2,"of Gaussian pyramid"
        pyramid.append(Reduce(pyramid[i]))
    print
    return pyramid

def LaplacianPyramid(I,n):
    '''
    generate Laplacian pyramid base on Gaussian pyramid on n levels

    param I: input image
    param n: levels of pyramid

    return: a list of image in Laplacian pyramid
    '''
    print ">>> Generating Laplacian pyramid with", n, "levels"
    pyramid = []
    gaussian_pyramid = GaussianPyramid(I,n)
    for i in range(1,n):
        expanded = Expand(gaussian_pyramid[i])
        if expanded.shape[0] != gaussian_pyramid[i-1].shape[0]:
            expanded = expanded[:-1,:,:]
        if expanded.shape[1] != gaussian_pyramid[i-1].shape[1]:
            expanded = expanded[:,:-1,:]
        pyramid.append(gaussian_pyramid[i-1]-expanded)
    pyramid.append(gaussian_pyramid[n-1])
    return pyramid

def Reconstruct(LI,n):
    '''
    reconstruct original image by adding up all differences and the base image in Laplacian pyramid

    param LI: Laplacian pyramid
    param n: levels of reconstruction

    return: image after reconstruction
    '''
    stacks = len(LI)
    if n > stacks:
        n = stacks
    stacks -= 1
    print ">>> Reconstructing image-Lv. 1"
    base_img = LI[-1]
    for i in range(1,n):
        print ">>> Reconstructing image-Lv.",i+1
        expanded = Expand(base_img)
        if expanded.shape[0] != LI[stacks-i].shape[0]:
            expanded = expanded[:-1,:,:]
        if expanded.shape[1] != LI[stacks-i].shape[1]:
            expanded = expanded[:,:-1,:]
        base_img = expanded + LI[stacks-i]
    print
    return base_img

def BlendImage(image_1, image_2, pixel_1=-1, pixel_2=-1):
    '''
    blend image base on n level Laplacian pyramid of both image and n level Gaussian pyramid of bit masks. And finally use reconstruction function to reconstruct the final image

    param image_1, image_2: input images to be blended
    param pixel_1, pixel_2: x value of points corresponse

    return: blended image
    '''
    img_1 = tools.load_image(image_1)
    img_2 = tools.load_image(image_2)

    img_height = img_1.shape[0]
    img_width_1 = img_1.shape[1]
    img_width_2 = img_2.shape[1]
    print img_width_1, img_width_2

    # pick width by clicking
    if pixel_1 < 0 or pixel_2 < 0:
        plt.imshow(cv2.cvtColor(img_1.astype('uint8'),cv2.COLOR_BGR2RGB))
        x,_ = plt.ginput(1)[0]
        plt.close('all')
        pixel_1 = x
        plt.imshow(cv2.cvtColor(img_2.astype('uint8'),cv2.COLOR_BGR2RGB))
        x,_ = plt.ginput(1)[0]
        plt.close('all')
        pixel_2 = x
    pixel_1 = int(img_width_1-pixel_1)
    pixel_2 = int(pixel_2)

    blend_width = max([img_width_1+img_width_2-pixel_1-pixel_2, img_width_1, img_width_2])

    left_width = img_width_1-pixel_1
    right_width = blend_width-img_width_2+pixel_2

    img_1 = np.hstack((img_1,np.zeros((img_height,blend_width-img_width_1,3))))
    img_2 = np.hstack((np.zeros((img_height,blend_width-img_width_2,3)),img_2))

    # bit-mask
    bit_mask_1 = np.hstack((np.ones((img_height,left_width,3)),
                            np.zeros((img_height,blend_width-left_width,3))))
    bit_mask_2 = np.hstack((np.zeros((img_height,right_width,3)),
                            np.ones((img_height,img_width_2-pixel_2,3))))

    # pyramid
    laplacian_img_1 = LaplacianPyramid(img_1,5)
    laplacian_img_2 = LaplacianPyramid(img_2,5)
    print ">>> Generating bit-masks for image 1"
    gaussian_mask_1 = GaussianPyramid(bit_mask_1,5)
    print ">>> Generating bit-masks for image 2"
    gaussian_mask_2 = GaussianPyramid(bit_mask_2,5)

    # blend
    blend_pyramid_1 = [img*mask for img,mask in zip(laplacian_img_1,gaussian_mask_1)]
    blend_pyramid_2 = [img*mask for img,mask in zip(laplacian_img_2,gaussian_mask_2)]

    blend_pyramid = [blend_1+blend_2
                    for blend_1,blend_2 in zip(blend_pyramid_1,blend_pyramid_2)]

    blended_img = Reconstruct(blend_pyramid,5)
    return blended_img

def GetParameters(pos_1, pos_2):
    '''
    using LSE to get affine parameters which can warp image 2 to image 1

    param pos_1, pos_2: corresponse points

    return: affine parameters
    '''
    target_vec = tools.target_vector(pos_1)
    affine_mat = tools.affine_matrix(pos_2)

    if len(pos_1) == 3:
        parameter_vec = np.linalg.inv(affine_mat).dot(target_vec)
    else:
        pseudo_inv = np.linalg.inv(affine_mat.T.dot(affine_mat)).dot(affine_mat.T)
        parameter_vec = pseudo_inv.dot(target_vec)

    parameter_f = parameter_vec.flatten()
    parameter_m = parameter_f[:4].reshape((2,2))
    parameter_t = parameter_f[4:].reshape((2,1))

    parameter_vec = np.hstack((parameter_m, parameter_t))

    return parameter_vec
