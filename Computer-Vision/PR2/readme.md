## How to Run

```
$ python main.py --left_image=<left image> --right_image=<right_image> --level=<pyramid level> --kernel_size=<kernel size> --window_length=<length of region to be check> --region/feature --measure=[ssd,sad,ncc] --valid
```

`--left_image/right_image`: image files;  
`--level`: level of image pyramid;  
`--kernel_size`: size of block to estimate error;  
`--window_length`: range to estimate error;  
`--region/feature`: whether using region based or feature based;  
`--measure`: method to estimate error: SSD, SAD, NCC;  
`--valid`: if cross validation is needed;