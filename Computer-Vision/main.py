###################################
#                                 #
# Author: Ruiqi Wang              #
# Date: Oct 20th, 2018            #
#                                 #
###################################

import sys

from tools import *
from functions import *

def hw_part1(args):
    '''
    homework 1 part 1
    '''

    # read args, and the args should be like this:
    # $python main.py part1 image1.png image2.png image3.png ...
    # the length of args is variable and more args means more images to be blended
    # but it mush no less than 2
    if len(args) <= 1:
        print "At least choose two pictures"
        exit(0)
    blended_img = BlendImage(args[0], args[1])
    for i in range(2,len(args)):
        blended_img = BlendImage(blended_img,args[i])

    tools.show_image(blended_img)

def hw_part2(args):
    '''
    homework 1 part 2
    '''

    # read args, and the args should be like this:
    # $python main.py part2 image1.png image2.png [points >= 3]
    # the arg in [] is optional indicating number of points user will choose
    # but it must no less than 3
    if len(args) < 2:
        print "Please pick two images"
        exit(0)
    if len(args) == 3:
        pos_num = int(args[2])
    else:
        pos_num = 3
    if pos_num < 3:
        print "No enough points"
        exit(0)
    img_1 = cv2.imread(args[0],1)
    img_2 = cv2.imread(args[1],1)

    # get affine parameters
    plt.imshow(cv2.cvtColor(img_1.astype('uint8'),cv2.COLOR_BGR2RGB))
    pos_1 = plt.ginput(pos_num)
    plt.close('all')
    plt.imshow(cv2.cvtColor(img_2.astype('uint8'),cv2.COLOR_BGR2RGB))
    pos_2 = plt.ginput(pos_num)
    plt.close('all')
    pos_1, pos_2, parameter = GetParameters(pos_1, pos_2)

    img_2_wrap = cv2.warpAffine(img_2, parameter, (img_2.shape[1]*2,img_2.shape[0]))
    blended = BlendImage(img_1, img_2_wrap,
                         np.max(pos_1,axis=0)[0],
                         tools.affine_point(np.max(pos_2,axis=0),parameter)[0])

    tools.show_image(blended)

def extra(args):
    '''
    homework 1 extra
    '''

    # read args, and the args should be like this:
    # $python main.py extra image1.png image2.png
    img_1 = cv2.imread(args[0], 1)
    img_2 = cv2.imread(args[1], 1)

    img_1_g = cv2.cvtColor(img_1, cv2.COLOR_BGR2GRAY)
    img_2_g = cv2.cvtColor(img_2, cv2.COLOR_BGR2GRAY)

    dst_1 = cv2.cornerHarris(img_1_g, 2, 5, 0.05)
    dst_2 = cv2.cornerHarris(img_2_g, 2, 5, 0.05)
    dst_1 = cv2.dilate(dst_1, None)
    dst_2 = cv2.dilate(dst_2, None)

    pos_1 = np.vstack((np.nonzero(dst_1>0.3*dst_1.max()))).T
    pos_2 = np.vstack((np.nonzero(dst_2>0.3*dst_2.max()))).T

    sorted(pos_1, key=lambda x: x[1])
    sorted(pos_2, key=lambda x: x[1])

    print "matching points... (%d x %d)" % (pos_1.shape[0], pos_2.shape[0])
    matchs_1, matchs_2 = tools.match_points(img_1_g, img_2_g, pos_1, pos_2)
    print "(%d x %d) points matched" % (len(matchs_1), len(matchs_2))

    parameter = GetParameters(matchs_1, matchs_2)
    img_2_wrap = cv2.warpAffine(img_2, parameter, (img_2.shape[1]*2,img_2.shape[0]))
    blended = BlendImage(img_1, img_2_wrap,
                        np.max(matchs_1,axis=0)[0],
                        tools.affine_point(np.max(matchs_2,axis=0),parameter)[0])

    tools.show_image(blended)

if __name__ == "__main__":
    args = sys.argv[2:]
    if sys.argv[1] == "part1":
        hw_part1(args)
    elif sys.argv[1] == "part2":
        hw_part2(args)
    elif sys.argv[1] == "extra":
        extra(args)
    else:
        print "please specify 'part1' 'part2' or 'extra'"
        print "python main.py <part of hw> [images...]"
