from paddleocr import TextRecognition

model = TextRecognition(model_name="PP-OCRv6_medium_rec")
output = model.predict(input="./test.png", batch_size=1)
for res in output:
    res.print()
