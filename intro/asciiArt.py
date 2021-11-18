import PIL.Image, numpy
import data

def get_threshold(px_array):
    total = [0, 0, 0]

    for pixel in px_array:
        for RGB in range(3):
            total[RGB] += pixel[RGB]

    for RGB in range(3):
        total[RGB] = total[RGB] // len(px_array)
    average = (total[0] + total[1] + total[2]) // 3

    return average

def to_num(matrix, chars_per_line, relative, negative):
    num_array = [0] * chars_per_line

    line_size = len(matrix)
    column_size = len(matrix[0])

    for i in range(line_size):
        for j in range(column_size):
            if negative:
                if matrix[i][j] < relative:
                    num_array[i // 2] = num_array[i // 2] + 2 ** ((j * 2) + (i % 2))
            else:
                if matrix[i][j] > relative:
                    num_array[i // 2] = num_array[i // 2] + 2 ** ((j * 2) + (i % 2))

    return num_array

def to_matrix(array, height_index, width):
    matrix = numpy.zeros((width, 4))
    array_index = 0 + height_index * width
    for j in range(4):
        for i in range(width):
            matrix[i][j] = array[array_index]
            array_index = array_index + 1
    return matrix

def px_ascii_art(array, width, height, relative, negative):

    width = width - width % 2
    height = height - height % 4
    number_of_lines = height // 4
    ascii_art = ""

    height_index = 0
    while number_of_lines > 0:
        matrix = to_matrix(array, height_index, width)
        num_array = to_num(matrix, width // 2, relative, negative)
        for number in num_array:
            ascii_art = ascii_art + chr(data.brailleData[number])

        ascii_art = ascii_art + "\n"
        height_index = height_index + 4
        number_of_lines = number_of_lines - 1

    return ascii_art

def resize_image(image, new_width):
    width, height = image.size
    new_height = int(new_width * (height / width))
    new_image = image.resize((new_width, new_height))
    return new_image

def main(img_num):
    new_image_width = 400
    threshold = 110
    negative = True
    file_path = "./intro/" + str(img_num) + ".jpg"

    try:
        image = PIL.Image.open(file_path)
    except:
        print(file_path + ': File not found')
        exit()

    image = resize_image(image, new_image_width)
    width, height = image.size
    ascii_art = ""

    if threshold == -1:
        px_array = image.getdata()
        threshold = get_threshold(px_array)
        
    px_array = image.convert("L").getdata()
    ascii_art = px_ascii_art(px_array, width, height, threshold, negative)

    output_file = open("./intro_ascii/" + str(img_num) + ".txt", 'w', encoding="UTF-8")
    output_file.write(ascii_art)

    print(str(i) + "번째 완료됨")

for i in range(1, 83):
    main(i)
