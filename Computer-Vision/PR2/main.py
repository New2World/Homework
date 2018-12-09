###################################
#                                 #
# Author: Ruiqi Wang              #
# Date: Dec 6th, 2018             #
#                                 #
###################################

import os
import numpy as np
import cv2
import argparse
import matplotlib.pyplot as plt

import tools, functions

def config():
    parser = argparse.ArgumentParser()
    parser.add_argument("--left_image", type=str)
    parser.add_argument("--right_image", type=str)
    parser.add_argument("--level", type=int)
    parser.add_argument("--kernel_size", type=int)
    parser.add_argument("--window_length", type=int)
    parser.add_argument("--region", dest="method", action="store_true")
    parser.add_argument("--feature", dest="method", action="store_false")
    parser.add_argument("--measure", type=str)
    parser.add_argument("--valid", dest="valid", action="store_true")

    parser.set_defaults(method=True)
    parser.set_defaults(valid=False)

    args = parser.parse_args()
    return args

def region_based(args, valid=False):
    img_l = tools.load_image(args.left_image, 0)
    img_r = tools.load_image(args.right_image, 0)
    if valid:
        img_l, img_r = img_r, img_l
    pyramid_l = functions.GaussianPyramid(img_l, args.level)
    pyramid_r = functions.GaussianPyramid(img_r, args.level)
    disparity = np.zeros(pyramid_l[-1].shape)
    for i in range(args.level - 1, -1, -1):
        print "### Lv.%d" % (i+1)
        disparity = disparity.astype(np.int)
        disparity = functions.GetDisparity_region(  pyramid_l[i],
                                                    pyramid_r[i],
                                                    args.kernel_size,
                                                    args.window_length,
                                                    args.measure,
                                                    disparity,
                                                    valid=valid)
        disparity = cv2.filter2D(disparity.astype(np.float32), -1, tools.mean_kernel2D)
        if i > 0:
            disparity = functions.Expand(disparity) * 2
    print
    return disparity.astype(np.int)

def feature_based(args, valid=False):
    img_l = tools.load_image(args.left_image, 0)
    img_r = tools.load_image(args.right_image, 0)
    if valid:
        img_l, img_r = img_r, img_l
    feat_l = tools.get_features(img_l)
    feat_r = tools.get_features(img_r)
    disparity = np.zeros(img_l.shape).astype(np.int)
    disparity = functions.GetDisparity_feature( img_l,
                                                img_r,
                                                args.kernel_size,
                                                feat_l,
                                                feat_r,
                                                args.measure,
                                                disparity,
                                                valid=valid)
    disparity = cv2.filter2D(disparity.astype(np.float32), -1, tools.mean_kernel2D)
    print
    return disparity.astype(np.int)

def main(args):
    print
    if args.method:
        disparity = region_based(args)
        cv2.imwrite(os.path.join(os.path.dirname(args.left_image), "disp.png"), disparity);
        if args.valid:
            disparity_valid = region_based(args, valid=True)
            cv2.imwrite(os.path.join(os.path.dirname(args.left_image), "disp_valid.png"), disparity_valid)
    else:
        disparity = feature_based(args)
        cv2.imwrite(os.path.join(os.path.dirname(args.left_image), "disp.png"), disparity);
        if args.valid:
            disparity_valid = feature_based(args, valid=True)
            cv2.imwrite(os.path.join(os.path.dirname(args.left_image), "disp_valid.png"), disparity_valid);

if __name__ == "__main__":
    args = config()
    main(args)