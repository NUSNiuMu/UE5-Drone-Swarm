import os
import time
import re
import sys
import argparse

WATCH_ROOT = r"D:/data/nm/picture cam"
YOLO_MODEL_PATH = "D:/ue5/Drone/runs/detect/train/weights/best.pt"
UE5_IP = "127.0.0.1"
UE5_PORT = 12345

from ultralytics import YOLO
import cv2
import socket
import json

def parse_arguments():
    parser = argparse.ArgumentParser(description='YOLO文件夹监视器 - 单无人机版本')
    parser.add_argument('--drone_id', type=int, required=True, 
                       help='要监视的无人机ID (例如: 1, 2, 3...)')
    parser.add_argument('--watch_root', type=str, default=WATCH_ROOT,
                       help='监视的根目录路径')
    parser.add_argument('--model_path', type=str, default=YOLO_MODEL_PATH,
                       help='YOLO模型路径')
    parser.add_argument('--ue5_ip', type=str, default=UE5_IP,
                       help='UE5服务器IP地址')
    parser.add_argument('--ue5_port', type=int, default=UE5_PORT,
                       help='UE5服务器端口')
    return parser.parse_args()

def send_result(result_dict, ue5_ip, ue5_port):
    msg = json.dumps(result_dict, ensure_ascii=False)
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"{result_dict}")
    sock.sendto(msg.encode('utf-8'), (ue5_ip, ue5_port))
    sock.close()

def wait_for_file_release(filepath, timeout=5):
    start = time.time()
    while True:
        try:
            with open(filepath, 'rb'):
                return True
        except PermissionError:
            if time.time() - start > timeout:
                return False
            time.sleep(0.1)

def draw_boxes_on_image(image, results, drone_id, model):
    """在图片上绘制检测框"""
    img_with_boxes = image.copy()
    for r in results:
        for box in r.boxes:
            cls = int(box.cls[0])
            label_name = model.names[cls]
            xyxy = [int(v) for v in box.xyxy[0].tolist()]
            conf = round(float(box.conf[0]), 2)
            # 绘制边界框
            cv2.rectangle(img_with_boxes, (xyxy[0], xyxy[1]), (xyxy[2], xyxy[3]), (0, 255, 0), 2)
            # 计算矩形中心点
            center_x = (xyxy[0] + xyxy[2]) // 2
            center_y = (xyxy[1] + xyxy[3]) // 2
            # 在矩形中心绘制蓝色点
            cv2.circle(img_with_boxes, (center_x, center_y), 5, (255, 0, 0), -1)
            # 绘制标签文本
            label_text = f"{label_name} {conf:.2f}"
            cv2.putText(img_with_boxes, label_text, (xyxy[0], xyxy[1] - 10), 
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
    # 添加drone_id信息
    cv2.putText(img_with_boxes, f"Drone ID: {drone_id}", (10, 30), 
                cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 2)
    return img_with_boxes

def save_image_with_boxes(image, original_path, drone_id, results, model):
    img_with_boxes = draw_boxes_on_image(image, results, drone_id, model)
    output_dir = os.path.join(os.path.dirname(original_path), "detected")
    os.makedirs(output_dir, exist_ok=True)
    base_name = os.path.basename(original_path)
    name_without_ext = os.path.splitext(base_name)[0]
    output_path = os.path.join(output_dir, f"{name_without_ext}_detected.png")
    cv2.imwrite(output_path, img_with_boxes)
    print(f"保存检测结果图片: {output_path}")

def watch_folder(folder_path, drone_id, model, ue5_ip, ue5_port):
    print(f"Watching folder: {folder_path} (drone_id={drone_id})")
    processed = set()
    while True:
        files = [f for f in os.listdir(folder_path) if f.endswith('.png')]
        files.sort()
        for fname in files:
            if fname in processed:
                continue
            img_path = os.path.join(folder_path, fname)
            if not wait_for_file_release(img_path, timeout=5):
                print(f"Timeout waiting for file: {img_path}")
                continue
            img = cv2.imread(img_path)
            if img is None:
                continue
            results = model(img)
            drones_results = []
            send_flag = False
            for r in results:
                for box in r.boxes:
                    cls = int(box.cls[0])
                    label_name = model.names[cls]
                    xyxy = [round(float(v), 2) for v in box.xyxy[0].tolist()]
                    conf = round(float(box.conf[0]), 2)
                    if conf > 0.4:
                        drones_results.append({
                            "drone_id": drone_id,
                            "label": label_name,
                            "box": xyxy,
                        })
                        send_flag = True  # 只要有一个大于0.5就准备发送
            # 保存带有检测框的图片
            if drones_results:
                save_image_with_boxes(img, img_path, drone_id, results, model)
            if send_flag:
                print("*"*50)
                send_result(drones_results, ue5_ip, ue5_port)
                print(f"Sent result for {fname} (检测到置信度>0.5即发送)")
                print("*"*50)
            processed.add(fname)
        time.sleep(0.05)

def main():
    args = parse_arguments()
    drone_id = args.drone_id
    print(f"启动单无人机监视器 - Drone ID: {drone_id}")
    print(f"监视根目录: {args.watch_root}")
    print(f"YOLO模型: {args.model_path}")
    print(f"UE5服务器: {args.ue5_ip}:{args.ue5_port}")
    # 加载YOLO模型
    print("正在加载YOLO模型...")
    model = YOLO(args.model_path)
    model.to('cpu')
    try:
        model.fuse()
        print("模型fuse完成")
    except AttributeError:
        print("模型不支持 fuse，已跳过 fuse 操作")
    # 查找对应的无人机文件夹（直接拼接路径）
    target_folder = os.path.join(args.watch_root, f"BP_DroneActor_C_{drone_id}")
    if not os.path.isdir(target_folder):
        print(f"错误: 未找到Drone ID {drone_id}对应的文件夹: {target_folder}")
        sys.exit(1)
    print(f"找到目标文件夹: {target_folder}")
    # 开始监视
    watch_folder(target_folder, drone_id, model, args.ue5_ip, args.ue5_port)

if __name__ == "__main__":
    main()