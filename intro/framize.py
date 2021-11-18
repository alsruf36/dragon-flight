import cv2
vidcap = cv2.VideoCapture('intro.mp4')
success,image = vidcap.read()

count = 1
success = True

while success:
  success,image = vidcap.read()
  cv2.imwrite("intro/%d.jpg" % count, image)
  print("saved image %d.jpg" % count)
  
  if cv2.waitKey(10) == 27:                    
      break
  count += 1