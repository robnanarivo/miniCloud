import React from "react";
import MenuBar from "./MenuBar";
import Folder from "./Folder";
import { useState } from "react";
import { Card, Col, Row } from "antd";
const { Meta } = Card;

const Storage = (props) => {
//   const roots = ["/"];
  const [roots, setRoots] = useState(["/"]);
  const sleep = ms => new Promise(res => setTimeout(res, ms));
  const refresh = async() => {
    setRoots([]);
    await sleep(500);
    setRoots(["/"]);
  }
  return (
    <>
      <div style={{ width: "70vw", margin: "0 auto", marginTop: "5vh" }}>
        <h4>Penn Cloud Storage</h4>
        <ul className="storage">
          {roots.length>0&&roots.map((root) => {
            const path = root.split("/").filter((p) => p);
            return (
              <Folder name={"root"} key={root} path={[]} refresh={()=>refresh()}/>
            );
          })}
        </ul>
      </div>
    </>
  );
};
export default Storage;
